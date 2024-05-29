#include <gtk/gtk.h>
#include <vte/vte.h>

static void on_child_exit(VteTerminal *terminal, gpointer user_data) {
    gtk_main_quit();
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // Create the main window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Enhanced VTE Terminal");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Create a VTE terminal widget
    VteTerminal *terminal = VTE_TERMINAL(vte_terminal_new());

    // Set font and colors
    vte_terminal_set_font(terminal, pango_font_description_from_string("Monospace 12"));
    GdkRGBA bg_color = {0.0, 0.0, 0.0, 1.0}; // Black
    GdkRGBA fg_color = {1.0, 1.0, 1.0, 1.0}; // White
    vte_terminal_set_colors(terminal, &fg_color, &bg_color, NULL, 0);

    // Set scrollback lines
    vte_terminal_set_scrollback_lines(terminal, 10000);

    // Fork a new child process running the shell
    char **env = g_get_environ();
    char *shell_argv[] = {g_strdup(g_getenv("SHELL")), NULL}; // Rename to shell_argv
    vte_terminal_spawn_async(terminal,
                             VTE_PTY_DEFAULT,
                             NULL, // Working directory
                             shell_argv, // Command
                             env,  // Environment
                             G_SPAWN_DEFAULT,
                             NULL, // Child setup function
                             NULL, // User data for child setup
                             NULL, // Destroy notify function
                             -1,   // Timeout
                             NULL, // Cancellable
                             NULL, // Callback
                             NULL  // User data for callback
    );
    g_strfreev(env);

    // Handle terminal child exit
    g_signal_connect(terminal, "child-exited", G_CALLBACK(on_child_exit), NULL);

    // Add the terminal widget to the window
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(terminal));

    // Show all widgets
    gtk_widget_show_all(window);

    // Start the GTK main loop
    gtk_main();

    return 0;
}

