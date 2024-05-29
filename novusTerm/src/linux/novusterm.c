#include <gtk/gtk.h>
#include <stdlib.h>
#include <unistd.h>
#include <pty.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define BUF_SIZE 256

static gboolean on_io_channel(GIOChannel *source, GIOCondition condition, gpointer data) {
    int master_fd = g_io_channel_unix_get_fd(source);
    GtkTextBuffer *buffer = GTK_TEXT_BUFFER(data);
    GtkTextIter end_iter;
    char buffer_data[BUF_SIZE];
    int n;

    if (condition & G_IO_IN) {
        n = read(master_fd, buffer_data, BUF_SIZE);
        if (n > 0) {
            gtk_text_buffer_get_end_iter(buffer, &end_iter);
            for (int i = 0; i < n; i++) {
                if (buffer_data[i] == '\r') {
                    // Replace carriage return with newline, but only if it's not followed by a newline
                    if (i + 1 < n && buffer_data[i + 1] != '\n') {
                        gtk_text_buffer_insert(buffer, &end_iter, "\n", 1);
                    }
                    continue;
                } else if (buffer_data[i] == '\n') {
                    // Handle newline
                    gtk_text_buffer_insert(buffer, &end_iter, "\n", 1);
                } else if (buffer_data[i] == 0x7F || buffer_data[i] == '\b') {
                    // Handle backspace
                    if (!gtk_text_iter_is_start(&end_iter)) {
                        GtkTextIter start_iter = end_iter;
                        gtk_text_iter_backward_char(&start_iter);
                        gtk_text_buffer_delete(buffer, &start_iter, &end_iter);
                    }
                } else if (buffer_data[i] == '\033') {
                    // Handle escape sequences
                    char esc_seq[BUF_SIZE] = {0};
                    int j = 0;
                    while (i < n && buffer_data[i] != 'm') {
                        esc_seq[j++] = buffer_data[i++];
                    }
                    esc_seq[j++] = buffer_data[i];
                    gtk_text_buffer_get_end_iter(buffer, &end_iter);
                    gtk_text_buffer_insert(buffer, &end_iter, esc_seq, j);
                } else {
                    // Handle normal character
                    gtk_text_buffer_insert(buffer, &end_iter, &buffer_data[i], 1);
                }
            }
        }
    }
    return TRUE;
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    int master_fd = GPOINTER_TO_INT(data);
    char c;

    if (event->keyval == GDK_KEY_Return) {
        c = '\n';
    } else if (event->keyval == GDK_KEY_BackSpace) {
        c = 0x7F;
    } else if (event->keyval < 256) {
        c = (char) event->keyval;
    } else {
        return FALSE; // Ignore non-ASCII keys
    }

    if (write(master_fd, &c, 1) == -1) {
        perror("write to master_fd");
    }

    return TRUE; // Do not propagate the event further
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // Create a new window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Simple Terminal");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Create a text view widget to display terminal output
    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_NONE);
    GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_container_add(GTK_CONTAINER(window), text_view);

    int master_fd, slave_fd;
    pid_t pid;

    if (openpty(&master_fd, &slave_fd, NULL, NULL, NULL) == -1) {
        perror("openpty");
        return EXIT_FAILURE;
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {  // Child process
        close(master_fd);
        setsid(); // Create a new session
        if (ioctl(slave_fd, TIOCSCTTY, NULL) == -1) {
            perror("ioctl TIOCSCTTY");
            return EXIT_FAILURE;
        }
        dup2(slave_fd, STDIN_FILENO);
        dup2(slave_fd, STDOUT_FILENO);
        dup2(slave_fd, STDERR_FILENO);
        close(slave_fd);
        execlp("/bin/bash", "bash", NULL);
        perror("execlp");
        return EXIT_FAILURE;
    } else {  // Parent process
        close(slave_fd);

        // Create an IO channel for the PTY master
        GIOChannel *io_channel = g_io_channel_unix_new(master_fd);
        g_io_add_watch(io_channel, G_IO_IN, (GIOFunc)on_io_channel, text_buffer);

        // Connect key press events to the text view
        g_signal_connect(text_view, "key-press-event", G_CALLBACK(on_key_press), GINT_TO_POINTER(master_fd));

        // Show all widgets
        gtk_widget_show_all(window);

        // Start the GTK main loop
        gtk_main();

        // Wait for the child process to exit
        waitpid(pid, NULL, 0);
    }

    return EXIT_SUCCESS;
}

