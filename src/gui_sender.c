#include <gtk/gtk.h>

typedef struct {
    GtkWidget *window;
    GtkWidget *file_label;
    GtkWidget *ip_entry;
    GtkWidget *status_label;
    char *selected_file;
} AppWidgets;

static void on_file_selected(
    GObject *source_object,
    GAsyncResult *result,
    gpointer user_data)
{
    AppWidgets *widgets = user_data;
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source_object);
    GFile *file = gtk_file_dialog_open_finish(dialog, result, NULL);

    if (file == NULL) {
        return;
    }

    g_free(widgets->selected_file);
    widgets->selected_file = g_file_get_path(file);

    gtk_label_set_text(
        GTK_LABEL(widgets->file_label),
        widgets->selected_file
    );

    gtk_label_set_text(
        GTK_LABEL(widgets->status_label),
        "File selected"
    );

    g_object_unref(file);
}
static void on_sender_finished(
    GObject *source_object,
    GAsyncResult *result,
    gpointer user_data)
{
    AppWidgets *widgets = user_data;
    GSubprocess *process = G_SUBPROCESS(source_object);
    GError *error = NULL;

    gboolean success = g_subprocess_wait_check_finish(
        process,
        result,
        &error
    );

    if (success) {
        gtk_label_set_text(
            GTK_LABEL(widgets->status_label),
            "File sent successfully!"
        );
    } else {
        gtk_label_set_text(
            GTK_LABEL(widgets->status_label),
            "File transfer failed"
        );

        if (error != NULL) {
            g_printerr("Sender error: %s\n", error->message);
            g_error_free(error);
        }
    }
}

static void on_browse_clicked(GtkButton *button, gpointer user_data)
{
    AppWidgets *widgets = user_data;

    (void)button;

    GtkFileDialog *dialog = gtk_file_dialog_new();

    gtk_file_dialog_set_title(dialog, "Choose a file");

    gtk_file_dialog_open(
        dialog,
        GTK_WINDOW(widgets->window),
        NULL,
        on_file_selected,
        widgets
    );

    g_object_unref(dialog);
}

static void on_send_clicked(GtkButton *button, gpointer user_data)
{
    AppWidgets *widgets = user_data;
    GError *error = NULL;

    (void)button;

    const char *ip_address =
        gtk_editable_get_text(GTK_EDITABLE(widgets->ip_entry));

    if (widgets->selected_file == NULL) {
        gtk_label_set_text(
            GTK_LABEL(widgets->status_label),
            "Please choose a file first"
        );
        return;
    }

    if (ip_address == NULL || ip_address[0] == '\0') {
        gtk_label_set_text(
            GTK_LABEL(widgets->status_label),
            "Please enter a receiver IP"
        );
        return;
    }

    /*
     * Currently sender uses 127.0.0.1 internally.
     * We will connect the IP field in the next stage.
     */
    const char *arguments[] = {
        "./build/sender",
        widgets->selected_file,
        NULL
    };

    GSubprocess *process = g_subprocess_newv(
        arguments,
        G_SUBPROCESS_FLAGS_NONE,
        &error
    );

    if (process == NULL) {
        gtk_label_set_text(
            GTK_LABEL(widgets->status_label),
            "Could not start sender"
        );

        if (error != NULL) {
            g_printerr("Could not start sender: %s\n", error->message);
            g_error_free(error);
        }

        return;
    }

    gtk_label_set_text(
        GTK_LABEL(widgets->status_label),
        "Sending file..."
    );

    g_subprocess_wait_check_async(
        process,
        NULL,
        on_sender_finished,
        widgets
    );

    g_object_unref(process);
}

static void activate(GtkApplication *app, gpointer user_data)
{
    (void)user_data;

    AppWidgets *widgets = g_new0(AppWidgets, 1);

    widgets->window = gtk_application_window_new(app);

    gtk_window_set_title(
        GTK_WINDOW(widgets->window),
        "CTransfer"
    );

    gtk_window_set_default_size(
        GTK_WINDOW(widgets->window),
        500,
        300
    );

    GtkWidget *main_box = gtk_box_new(
        GTK_ORIENTATION_VERTICAL,
        12
    );

    gtk_widget_set_margin_top(main_box, 20);
    gtk_widget_set_margin_bottom(main_box, 20);
    gtk_widget_set_margin_start(main_box, 20);
    gtk_widget_set_margin_end(main_box, 20);

    GtkWidget *title = gtk_label_new(NULL);

    gtk_label_set_markup(
        GTK_LABEL(title),
        "<span size='x-large' weight='bold'>CTransfer</span>"
    );

    gtk_box_append(GTK_BOX(main_box), title);

    GtkWidget *ip_label = gtk_label_new("Receiver IP:");

    gtk_widget_set_halign(ip_label, GTK_ALIGN_START);

    gtk_box_append(GTK_BOX(main_box), ip_label);

    widgets->ip_entry = gtk_entry_new();

    gtk_editable_set_text(
        GTK_EDITABLE(widgets->ip_entry),
        "127.0.0.1"
    );

    gtk_box_append(GTK_BOX(main_box), widgets->ip_entry);

    GtkWidget *browse_button =
        gtk_button_new_with_label("Choose File");

    g_signal_connect(
        browse_button,
        "clicked",
        G_CALLBACK(on_browse_clicked),
        widgets
    );

    gtk_box_append(GTK_BOX(main_box), browse_button);

    widgets->file_label =
        gtk_label_new("No file selected");

    gtk_label_set_wrap(
        GTK_LABEL(widgets->file_label),
        TRUE
    );

    gtk_box_append(
        GTK_BOX(main_box),
        widgets->file_label
    );

    GtkWidget *send_button =
        gtk_button_new_with_label("Send File");

    g_signal_connect(
        send_button,
        "clicked",
        G_CALLBACK(on_send_clicked),
        widgets
    );

    gtk_box_append(GTK_BOX(main_box), send_button);

    widgets->status_label =
        gtk_label_new("Status: waiting");

    gtk_box_append(
        GTK_BOX(main_box),
        widgets->status_label
    );

    gtk_window_set_child(
        GTK_WINDOW(widgets->window),
        main_box
    );

    gtk_window_present(GTK_WINDOW(widgets->window));
}

int main(int argc, char *argv[])
{
    GtkApplication *app;
    int status;

    app = gtk_application_new(
        "com.reut.ctransfer",
        G_APPLICATION_DEFAULT_FLAGS
    );

    g_signal_connect(
        app,
        "activate",
        G_CALLBACK(activate),
        NULL
    );

    status = g_application_run(
        G_APPLICATION(app),
        argc,
        argv
    );

    g_object_unref(app);

    return status;
}