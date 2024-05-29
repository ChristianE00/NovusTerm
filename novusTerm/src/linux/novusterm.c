#include <gtk/gtk.h>
#include <vte/vte.h>

static void on_child_exit(VteTerminal *terminal, gint status, gpointer user_data) {
    GtkWidget *parent = gtk_widget_get_parent(GTK_WIDGET(terminal));

    if (GTK_IS_NOTEBOOK(parent)) {
        gint page_num = gtk_notebook_page_num(GTK_NOTEBOOK(parent), GTK_WIDGET(terminal));
        gtk_notebook_remove_page(GTK_NOTEBOOK(parent), page_num);
    } else if (GTK_IS_PANED(parent)) {
        GtkWidget *remaining_child = gtk_paned_get_child1(GTK_PANED(parent));
        if (remaining_child == GTK_WIDGET(terminal)) {
            remaining_child = gtk_paned_get_child2(GTK_PANED(parent));
        }

        GtkWidget *grandparent = gtk_widget_get_parent(parent);
        if (remaining_child != NULL) {
            gtk_container_remove(GTK_CONTAINER(parent), remaining_child);

            if (GTK_IS_NOTEBOOK(grandparent)) {
                gint page_num = gtk_notebook_page_num(GTK_NOTEBOOK(grandparent), parent);
                gtk_notebook_remove_page(GTK_NOTEBOOK(grandparent), page_num);
                gtk_notebook_insert_page(GTK_NOTEBOOK(grandparent), remaining_child, gtk_label_new("Terminal"), page_num);
            } else if (GTK_IS_PANED(grandparent)) {
                if (gtk_paned_get_child1(GTK_PANED(grandparent)) == parent) {
                    gtk_paned_pack1(GTK_PANED(grandparent), remaining_child, TRUE, FALSE);
                } else {
                    gtk_paned_pack2(GTK_PANED(grandparent), remaining_child, TRUE, FALSE);
                }
            } else {
                gtk_container_remove(GTK_CONTAINER(grandparent), parent);
                gtk_widget_destroy(parent);
            }
        } else {
            if (GTK_IS_NOTEBOOK(grandparent)) {
                gint page_num = gtk_notebook_page_num(GTK_NOTEBOOK(grandparent), parent);
                gtk_notebook_remove_page(GTK_NOTEBOOK(grandparent), page_num);
            } else if (GTK_IS_PANED(grandparent)) {
                gtk_container_remove(GTK_CONTAINER(grandparent), parent);
            }
            gtk_widget_destroy(parent);
        }
    } else {
        gtk_widget_destroy(GTK_WIDGET(terminal));
    }
    gtk_widget_show_all(gtk_widget_get_toplevel(GTK_WIDGET(terminal)));
}

static GtkWidget* create_terminal() {
    VteTerminal *terminal = VTE_TERMINAL(vte_terminal_new());

    const char *font_name = "DejaVu Sans Mono 12";
    PangoFontDescription *font_desc = pango_font_description_from_string(font_name);
    vte_terminal_set_font(terminal, font_desc);
    pango_font_description_free(font_desc);

    GdkRGBA fg_color = {0.82, 0.82, 0.82, 1.0};
    GdkRGBA bg_color = {0.12, 0.12, 0.12, 0.9};
    vte_terminal_set_colors(terminal, &fg_color, &bg_color, NULL, 0);
    vte_terminal_set_cursor_blink_mode(terminal, VTE_CURSOR_BLINK_ON);
    vte_terminal_set_cursor_shape(terminal, VTE_CURSOR_SHAPE_BLOCK);
    vte_terminal_set_scrollback_lines(terminal, 10000);
    vte_terminal_set_allow_hyperlink(terminal, TRUE);
    vte_terminal_set_mouse_autohide(terminal, TRUE);

    char **env = g_get_environ();
    char *shell_argv[] = {g_strdup(g_getenv("SHELL")), NULL};
    vte_terminal_spawn_async(terminal,
                             VTE_PTY_DEFAULT,
                             NULL,
                             shell_argv,
                             env,
                             G_SPAWN_DEFAULT,
                             NULL,
                             NULL,
                             NULL,
                             -1,
                             NULL,
                             NULL,
                             NULL);
    g_strfreev(env);

    g_signal_connect(terminal, "child-exited", G_CALLBACK(on_child_exit), NULL);

    return GTK_WIDGET(terminal);
}

static void on_new_tab(GtkButton *button, gpointer notebook) {
    GtkWidget *terminal = create_terminal();
    GtkWidget *label = gtk_label_new("Terminal");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), terminal, label);
    gtk_widget_show_all(GTK_WIDGET(notebook));
}

static void replace_with_pane(GtkNotebook *notebook, GtkWidget *pane, int page_num, GtkWidget *current_page) {
    GtkWidget *label = gtk_notebook_get_tab_label(notebook, current_page);
    g_object_ref(current_page); // Reference the widget to prevent it from being destroyed
    g_object_ref(label); // Reference the label to prevent it from being destroyed
    gtk_notebook_remove_page(notebook, page_num);
    if (label == NULL) {
        label = gtk_label_new("Terminal");
    }
    gtk_paned_pack1(GTK_PANED(pane), current_page, TRUE, FALSE);
    gtk_notebook_insert_page(GTK_NOTEBOOK(notebook), pane, label, page_num);
    g_object_unref(current_page); // Unreference the widget
    g_object_unref(label); // Unreference the label
    gtk_widget_show_all(GTK_WIDGET(notebook));
}

static void on_split_horizontally(GtkButton *button, gpointer notebook) {
    int current_page_num = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
    GtkWidget *current_page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), current_page_num);
    GtkWidget *new_terminal = create_terminal();

    GtkWidget *pane = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_pack2(GTK_PANED(pane), new_terminal, TRUE, FALSE);

    replace_with_pane(GTK_NOTEBOOK(notebook), pane, current_page_num, current_page);
}

static void on_split_vertically(GtkButton *button, gpointer notebook) {
    int current_page_num = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
    GtkWidget *current_page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), current_page_num);
    GtkWidget *new_terminal = create_terminal();

    GtkWidget *pane = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    gtk_paned_pack2(GTK_PANED(pane), new_terminal, TRUE, FALSE);

    replace_with_pane(GTK_NOTEBOOK(notebook), pane, current_page_num, current_page);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "NovusTerm");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *header_bar = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header_bar), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(header_bar), "NovusTerm");
    gtk_window_set_titlebar(GTK_WINDOW(window), header_bar);

    GtkWidget *notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
    gtk_container_add(GTK_CONTAINER(window), notebook);

    GtkWidget *new_tab_button = gtk_button_new_with_label("New Tab");
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header_bar), new_tab_button);
    g_signal_connect(new_tab_button, "clicked", G_CALLBACK(on_new_tab), notebook);

    GtkWidget *split_h_button = gtk_button_new_with_label("Split Horizontally");
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header_bar), split_h_button);
    g_signal_connect(split_h_button, "clicked", G_CALLBACK(on_split_horizontally), notebook);

    GtkWidget *split_v_button = gtk_button_new_with_label("Split Vertically");
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header_bar), split_v_button);
    g_signal_connect(split_v_button, "clicked", G_CALLBACK(on_split_vertically), notebook);

    GtkWidget *initial_terminal = create_terminal();
    GtkWidget *label = gtk_label_new("Terminal");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), initial_terminal, label);
    gtk_widget_show_all(GTK_WIDGET(notebook));

    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}
