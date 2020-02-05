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
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>

#include "toggle.h"
#include "toggle-dialogs.h"

/* default settings */
#define DEFAULT_ACTIVE_ICON "starred"
#define DEFAULT_INACTIVE_ICON "non-starred"
#define DEFAULT_PRIMARY_COMMAND "notify-send 'primary command executed'"
#define DEFAULT_SECONDARY_COMMAND "notify-send 'secondary command executed'"

static void
toggle_construct (XfcePanelPlugin *plugin);

XFCE_PANEL_PLUGIN_REGISTER (toggle_construct);



void
toggle_update_icon (TogglePlugin *toggle)
{
  GtkWidget *icon;

  icon = gtk_image_new_from_icon_name (toggle->active ?
                                         toggle->active_icon :
                                         toggle->inactive_icon,
                                       GTK_ICON_SIZE_BUTTON);

  gtk_button_set_image (GTK_BUTTON (toggle->button), icon);
}



void
toggle_save (XfcePanelPlugin *plugin,
             TogglePlugin    *toggle)
{
  XfceRc *rc;
  gchar  *file;

  file = xfce_panel_plugin_save_location (plugin, TRUE);

  if (G_UNLIKELY (file == NULL))
    {
       DBG ("Failed to open config file");
       return;
    }

  rc = xfce_rc_simple_open (file, FALSE);
  g_free (file);

  if (G_LIKELY (rc != NULL))
    {
      if (toggle->active_icon)
        xfce_rc_write_entry (rc, "active_icon", toggle->active_icon);

      if (toggle->inactive_icon)
        xfce_rc_write_entry (rc, "inactive_icon", toggle->inactive_icon);

      if (toggle->primary_command)
        xfce_rc_write_entry (rc, "primary_command", toggle->primary_command);

      if (toggle->secondary_command)
        xfce_rc_write_entry (rc, "secondary_command", toggle->secondary_command);

      xfce_rc_write_bool_entry (rc, "active", toggle->active);

      xfce_rc_close (rc);
    }
}



static void
toggle_read (TogglePlugin *toggle)
{
  XfceRc      *rc;
  gchar       *file;
  const gchar *value;

  file = xfce_panel_plugin_save_location (toggle->plugin, TRUE);

  if (G_LIKELY (file != NULL))
    {
      rc = xfce_rc_simple_open (file, TRUE);

      g_free (file);

      if (G_LIKELY (rc != NULL))
        {
          /* read the settings */
          value = xfce_rc_read_entry (rc, "active_icon", DEFAULT_ACTIVE_ICON);
          toggle->active_icon = g_strdup (value);

          value = xfce_rc_read_entry (rc, "inactive_icon", DEFAULT_INACTIVE_ICON);
          toggle->inactive_icon = g_strdup (value);

          value = xfce_rc_read_entry (rc, "primary_command", DEFAULT_PRIMARY_COMMAND);
          toggle->primary_command = g_strdup (value);

          value = xfce_rc_read_entry (rc, "secondary_command", DEFAULT_SECONDARY_COMMAND);
          toggle->secondary_command = g_strdup (value);

          toggle->active = xfce_rc_read_bool_entry (rc, "active", TRUE);

          xfce_rc_close (rc);

          return;
        }
    }

  /* something went wrong, apply default values */
  DBG ("Applying default settings");

  toggle->active_icon = g_strdup (DEFAULT_ACTIVE_ICON);
  toggle->inactive_icon = g_strdup (DEFAULT_INACTIVE_ICON);
  toggle->primary_command = g_strdup (DEFAULT_PRIMARY_COMMAND);
  toggle->secondary_command = g_strdup (DEFAULT_SECONDARY_COMMAND);
  toggle->active = TRUE;
}



static gboolean
toggle_pressed (GtkButton       *button,
                GdkEventButton  *event,
                TogglePlugin    *toggle)
{
  if (event->button == 1)
    {
      g_spawn_command_line_async (toggle->primary_command, NULL);
      toggle->active = !toggle->active;
      toggle_update_icon (toggle);
      return TRUE;
    }

  if (event->button == 2)
    {
      g_spawn_command_line_async (toggle->secondary_command, NULL);
      return TRUE;
    }

  return FALSE;
}



static TogglePlugin *
toggle_new (XfcePanelPlugin *plugin)
{
  TogglePlugin   *toggle;
  GtkOrientation  orientation;

  toggle = g_slice_new0 (TogglePlugin);

  toggle->plugin = plugin;

  toggle_read (toggle);

  orientation = xfce_panel_plugin_get_orientation (plugin);

  toggle->ebox = gtk_event_box_new ();
  gtk_widget_show (toggle->ebox);

  toggle->hvbox = gtk_box_new (orientation, 2);
  gtk_widget_show (toggle->hvbox);
  gtk_container_add (GTK_CONTAINER (toggle->ebox), toggle->hvbox);

  toggle->button = gtk_button_new ();
  gtk_widget_show (toggle->button);
  gtk_box_pack_start (GTK_BOX (toggle->hvbox), toggle->button, FALSE, FALSE, 0);

  toggle_update_icon (toggle);

  g_signal_connect (G_OBJECT (toggle->button), "button-press-event",
                    G_CALLBACK (toggle_pressed), toggle);

  return toggle;
}



static void
toggle_free (XfcePanelPlugin *plugin,
             TogglePlugin    *toggle)
{
  GtkWidget *dialog;

  dialog = g_object_get_data (G_OBJECT (plugin), "dialog");
  if (G_UNLIKELY (dialog != NULL))
    gtk_widget_destroy (dialog);

  gtk_widget_destroy (toggle->hvbox);

  /* cleanup the settings */
  if (G_LIKELY (toggle->active_icon != NULL))
    g_free (toggle->active_icon);

  if (G_LIKELY (toggle->inactive_icon != NULL))
    g_free (toggle->inactive_icon);

  if (G_LIKELY (toggle->primary_command != NULL))
    g_free (toggle->primary_command);

  if (G_LIKELY (toggle->secondary_command != NULL))
    g_free (toggle->secondary_command);

  g_slice_free (TogglePlugin, toggle);
}



static void
toggle_orientation_changed (XfcePanelPlugin *plugin,
                            GtkOrientation   orientation,
                            TogglePlugin    *toggle)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (toggle->hvbox), orientation);
}



static gboolean
toggle_size_changed (XfcePanelPlugin *plugin,
                     gint             size,
                     TogglePlugin    *toggle)
{
  GtkOrientation orientation;

  orientation = xfce_panel_plugin_get_orientation (plugin);

  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, size);
  else
    gtk_widget_set_size_request (GTK_WIDGET (plugin), size, -1);

  return TRUE;
}



static void
toggle_construct (XfcePanelPlugin *plugin)
{
  TogglePlugin *toggle;

  xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  toggle = toggle_new (plugin);

  gtk_container_add (GTK_CONTAINER (plugin), toggle->ebox);

  xfce_panel_plugin_add_action_widget (plugin, toggle->ebox);

  g_signal_connect (G_OBJECT (plugin), "free-data",
                    G_CALLBACK (toggle_free), toggle);

  g_signal_connect (G_OBJECT (plugin), "save",
                    G_CALLBACK (toggle_save), toggle);

  g_signal_connect (G_OBJECT (plugin), "size-changed",
                    G_CALLBACK (toggle_size_changed), toggle);

  g_signal_connect (G_OBJECT (plugin), "orientation-changed",
                    G_CALLBACK (toggle_orientation_changed), toggle);

  xfce_panel_plugin_menu_show_configure (plugin);
  g_signal_connect (G_OBJECT (plugin), "configure-plugin",
                    G_CALLBACK (toggle_configure), toggle);

  xfce_panel_plugin_menu_show_about (plugin);
  g_signal_connect (G_OBJECT (plugin), "about",
                    G_CALLBACK (toggle_about), NULL);
}
