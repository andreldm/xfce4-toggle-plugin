/*  $Id$
 *
 *  Copyright (C) 2020 Andre Miranda <andreldm@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <gtk/gtk.h>

#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4panel/libxfce4panel.h>

#include "toggle.h"
#include "toggle-dialogs.h"

/* the website url */
#define PLUGIN_WEBSITE "https://docs.xfce.org/panel-plugins/xfce4-sample-plugin"

static GtkWidget *active_icon_entry;
static GtkWidget *inactive_icon_entry;
static GtkWidget *primary_command_entry;
static GtkWidget *secondary_command_entry;

static void
toggle_configure_response (GtkWidget    *dialog,
                           gint          response,
                           TogglePlugin *toggle)
{
  gboolean result;

  if (response == GTK_RESPONSE_HELP)
    {
      result = g_spawn_command_line_async ("exo-open --launch WebBrowser " PLUGIN_WEBSITE, NULL);

      if (G_UNLIKELY (result == FALSE))
        g_warning (_("Unable to open the following url: %s"), PLUGIN_WEBSITE);
    }
  else
    {
      g_object_set_data (G_OBJECT (toggle->plugin), "dialog", NULL);

      xfce_panel_plugin_unblock_menu (toggle->plugin);

      if (G_LIKELY (active_icon_entry)) {
        toggle->active_icon = g_strdup (gtk_entry_get_text (GTK_ENTRY (active_icon_entry)));
        active_icon_entry = NULL;
      }

      if (G_LIKELY (inactive_icon_entry)) {
        toggle->inactive_icon = g_strdup (gtk_entry_get_text (GTK_ENTRY (inactive_icon_entry)));
        inactive_icon_entry = NULL;
      }

      if (G_LIKELY (primary_command_entry)) {
        toggle->primary_command = g_strdup (gtk_entry_get_text (GTK_ENTRY (primary_command_entry)));
        primary_command_entry = NULL;
      }

      if (G_LIKELY (secondary_command_entry)) {
        toggle->secondary_command = g_strdup (gtk_entry_get_text (GTK_ENTRY (secondary_command_entry)));
        secondary_command_entry = NULL;
      }

      toggle_save (toggle->plugin, toggle);

      toggle_update_icon (toggle);

      gtk_widget_destroy (dialog);
    }
}



void
toggle_configure (XfcePanelPlugin *plugin,
                  TogglePlugin    *toggle)
{
  GtkWidget *dialog;
  GtkWidget *grid, *label;

  xfce_panel_plugin_block_menu (plugin);

  dialog = xfce_titled_dialog_new_with_buttons (_("Toggle Plugin"),
                                                GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                                                GTK_DIALOG_DESTROY_WITH_PARENT,
                                                "gtk-help", GTK_RESPONSE_HELP,
                                                "gtk-close", GTK_RESPONSE_OK,
                                                NULL);

  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_icon_name (GTK_WINDOW (dialog), "xfce4-settings");

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
  gtk_widget_set_margin_start (grid, 12);
  gtk_widget_set_margin_end (grid, 12);
  gtk_widget_set_margin_top (grid, 12);
  gtk_widget_set_margin_bottom (grid, 12);
  gtk_container_add_with_properties (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                                     grid, "expand", TRUE, "fill", TRUE, NULL);

  /* Active icon */
  label = gtk_label_new (_("Active icon"));
  gtk_label_set_xalign (GTK_LABEL (label), 0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);

  active_icon_entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (active_icon_entry), toggle->active_icon);
  gtk_grid_attach (GTK_GRID (grid), active_icon_entry, 1, 0, 1, 1);

  /* Inactive icon */
  label = gtk_label_new (_("Inactive icon"));
  gtk_label_set_xalign (GTK_LABEL (label), 0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);

  inactive_icon_entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (inactive_icon_entry), toggle->inactive_icon);
  gtk_grid_attach (GTK_GRID (grid), inactive_icon_entry, 1, 1, 1, 1);

  /* Primary command */
  label = gtk_label_new (_("Primary command (left click)"));
  gtk_label_set_xalign (GTK_LABEL (label), 0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 2, 1, 1);

  primary_command_entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (primary_command_entry), toggle->primary_command);
  gtk_grid_attach (GTK_GRID (grid), primary_command_entry, 1, 2, 1, 1);

  /* Secondary command */
  label = gtk_label_new (_("Secondary command (middle click)"));
  gtk_label_set_xalign (GTK_LABEL (label), 0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 3, 1, 1);

  secondary_command_entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (secondary_command_entry), toggle->secondary_command);
  gtk_grid_attach (GTK_GRID (grid), secondary_command_entry, 1, 3, 1, 1);

  g_object_set_data (G_OBJECT (plugin), "dialog", dialog);
  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (toggle_configure_response), toggle);

  gtk_widget_show_all (dialog);
}



void
toggle_about (XfcePanelPlugin *plugin)
{
  GdkPixbuf *icon;

  const gchar *auth[] =
    {
      "Andre Miranda <andreldm@xfce.org>",
      NULL
    };

  icon = xfce_panel_pixbuf_from_source ("xfce4-toggle-plugin", NULL, 32);
  gtk_show_about_dialog (NULL,
                         "logo",         icon,
                         "license",      xfce_get_license_text (XFCE_LICENSE_TEXT_GPL),
                         "version",      PACKAGE_VERSION,
                         "program-name", PACKAGE_NAME,
                         "comments",     _("An Xfce panel plugin that acts as a toggle switch"),
                         "website",      PLUGIN_WEBSITE,
                         "copyright",    _("Copyright \xc2\xa9 2020 Andre Miranda\n"),
                         "authors",      auth,
                         NULL);

  if (icon)
    g_object_unref (G_OBJECT (icon));
}
