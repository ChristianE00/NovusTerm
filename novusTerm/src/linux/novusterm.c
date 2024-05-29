#include <gtk/gtk.h>
#include <vte/vte.h>

static void on_child_exit(VteTerminal *terminal, gpointer user_data) {
    gtk_main_quit();
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // Create the main window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "NovusTerm");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Create a header bar
    GtkWidget *header_bar = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header_bar), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(header_bar), "NovusTerm");
    gtk_window_set_titlebar(GTK_WINDOW(window), header_bar);

    // Create a VTE terminal widget
    VteTerminal *terminal = VTE_TERMINAL(vte_terminal_new());

    // Set font
    const char *font_name = "DejaVu Sans Mono 12";
    PangoFontDescription *font_desc = pango_font_description_from_string(font_name);
    vte_terminal_set_font(terminal, font_desc);
    pango_font_description_free(font_desc);

    // Set colors
    GdkRGBA fg_color = {0.82, 0.82, 0.82, 1.0}; // Light Gray
    GdkRGBA bg_color = {0.12, 0.12, 0.12, 0.9}; // Almost Black with slight transparency
    GdkRGBA cursor_color = {0.0, 1.0, 0.0, 1.0}; // Green
    GdkRGBA palette[16] = {
        {0, 0, 0, 1}, {1, 0, 0, 1}, {0, 1, 0, 1}, {1, 1, 0, 1}, // Black, Red, Green, Yellow
        {0, 0, 1, 1}, {1, 0, 1, 1}, {0, 1, 1, 1}, {1, 1, 1, 1}, // Blue, Magenta, Cyan, White
        {0.5, 0.5, 0.5, 1}, {1, 0.5, 0.5, 1}, {0.5, 1, 0.5, 1}, {1, 1, 0.5, 1}, // Bright Black, Bright Red, Bright Green, Bright Yellow
        {0.5, 0.5, 1, 1}, {1, 0.5, 1, 1}, {0.5, 1, 1, 1}, {1, 1, 1, 1}  // Bright Blue, Bright Magenta, Bright Cyan, Bright White
    };
    vte_terminal_set_colors(terminal, &fg_color, &bg_color, palette, 16);
    vte_terminal_set_cursor_blink_mode(terminal, VTE_CURSOR_BLINK_ON);
    vte_terminal_set_cursor_shape(terminal, VTE_CURSOR_SHAPE_BLOCK);

    // Set scrollback lines
    vte_terminal_set_scrollback_lines(terminal, 10000);

    // Enable hyperlink support
    vte_terminal_set_allow_hyperlink(terminal, TRUE);

    // Enable mouse autohide
    vte_terminal_set_mouse_autohide(terminal, TRUE);

    // Fork a new child process running the shell
    char **env = g_get_environ();
    char *shell_argv[] = {g_strdup(g_getenv("SHELL")), NULL};
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

