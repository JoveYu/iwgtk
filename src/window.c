/*
 *  Copyright 2020 Jesse Lentz
 *
 *  This file is part of iwgtk.
 *
 *  iwgtk is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  iwgtk is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with iwgtk.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "iwgtk.h"

ObjectMethods object_methods[] = {
    {IWD_IFACE_KNOWN_NETWORK, (ConstructorFunction) known_network_add, (DestructorFunction) known_network_remove, NULL},
    {IWD_IFACE_ADAPTER,       (ConstructorFunction) adapter_add,       (DestructorFunction) adapter_remove,       NULL},
    {IWD_IFACE_DEVICE,        (ConstructorFunction) device_add,        (DestructorFunction) device_remove,        NULL},
    {IWD_IFACE_STATION,       (ConstructorFunction) station_add,       (DestructorFunction) station_remove,       indicator_set_station},
    {IWD_IFACE_AP,            (ConstructorFunction) ap_add,            (DestructorFunction) ap_remove,            indicator_set_ap},
    {IWD_IFACE_AD_HOC,        (ConstructorFunction) adhoc_add,         (DestructorFunction) adhoc_remove,         indicator_set_adhoc},
    {IWD_IFACE_WPS,           (ConstructorFunction) wps_add,           (DestructorFunction) wps_remove,           NULL},
    {IWD_IFACE_NETWORK,       (ConstructorFunction) network_add,       (DestructorFunction) network_remove,       NULL}
};

CoupleMethods couple_methods[] = {
    {(BindFunction) bind_adapter_device, (UnbindFunction) unbind_adapter_device},
    {(BindFunction) bind_device_station, (UnbindFunction) unbind_device_station},
    {(BindFunction) bind_device_ap,      (UnbindFunction) unbind_device_ap},
    {(BindFunction) bind_device_adhoc,   (UnbindFunction) unbind_device_adhoc},
    {(BindFunction) bind_device_wps,     (UnbindFunction) unbind_device_wps}
};

void window_new() {
    if (global.window != NULL) {
	/*
	 * Ideally, we should use gtk_window_present() here.
	 * But gtk_window_present() seems to do nothing in Sway 1.5.
	 * As an ugly workaround, destroy the existing window before creating a new one.
	 */
	gtk_widget_destroy(global.window->window);
    }

    global.window = malloc(sizeof(Window));

    memset(global.window->objects, 0, sizeof(void *) * n_object_types);
    memset(global.window->couples, 0, sizeof(void *) * n_couple_types);

    global.window->window = gtk_application_window_new(global.application);
    gtk_window_set_title(GTK_WINDOW(global.window->window), "iwgtk");
    gtk_window_set_default_size(GTK_WINDOW(global.window->window), 600, 700);
    gtk_window_set_icon_name(GTK_WINDOW(global.window->window), "iwgtk");

    window_set();
}

void window_set() {
    Window *window;

    window = global.window;

    bin_empty(GTK_BIN(window->window));

    if (!global.manager) {
	GtkWidget *label;

	/*
	 * TODO:
	 * Replace this with a more informative message.
	 */
	label = gtk_label_new("iwd is not running");

	gtk_container_add(GTK_CONTAINER(window->window), label);
	gtk_widget_show_all(window->window);
	return;
    }

    window->master = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    g_object_ref_sink(window->master);
    gtk_container_add(GTK_CONTAINER(window->window), window->master);

    window->header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    g_object_ref_sink(window->header);
    gtk_box_pack_start(GTK_BOX(window->master), window->header, FALSE, FALSE, 0);

    gtk_widget_set_margin_start(window->header, 5);
    gtk_widget_set_margin_end(window->header, 5);
    gtk_widget_set_margin_top(window->header, 5);

    gtk_box_pack_start(GTK_BOX(window->master), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 0);

    window->main = gtk_scrolled_window_new(NULL, NULL);
    g_object_ref_sink(window->main);
    gtk_scrolled_window_set_propagate_natural_height(GTK_SCROLLED_WINDOW(window->main), TRUE);
    gtk_box_pack_start(GTK_BOX(window->master), window->main, FALSE, FALSE, 0);

    window->known_network_button = gtk_radio_button_new_with_label(NULL, "Known Networks");
    g_object_ref_sink(window->known_network_button);
    gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(window->known_network_button), FALSE);

    {
	GtkWidget *known_network_button_vbox;

	known_network_button_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_end(GTK_BOX(window->header), known_network_button_vbox, FALSE, FALSE, 0);
	{
	    GtkWidget *quit_button;

	    quit_button = gtk_button_new_with_label("Quit");
	    g_signal_connect(quit_button, "clicked", G_CALLBACK(iwgtk_quit), NULL);
	    gtk_box_pack_start(GTK_BOX(known_network_button_vbox), quit_button, FALSE, FALSE, 0);
	}
	gtk_box_pack_end(GTK_BOX(known_network_button_vbox), window->known_network_button, FALSE, FALSE, 0);
    }

    g_signal_connect(window->known_network_button, "toggled", G_CALLBACK(known_network_table_show), window);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(window->known_network_button), TRUE);

    window->known_network_table = gtk_grid_new();
    g_object_ref_sink(window->known_network_table);
    gtk_container_add(GTK_CONTAINER(window->main), window->known_network_table);

    gtk_widget_set_margin_start(window->known_network_table, 5);
    gtk_widget_set_margin_end(window->known_network_table, 5);
    gtk_widget_set_margin_bottom(window->known_network_table, 5);

    gtk_grid_attach(GTK_GRID(window->known_network_table), new_label_bold("SSID"),            0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(window->known_network_table), new_label_bold("Security"),        1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(window->known_network_table), new_label_bold("Autoconnect"),     2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(window->known_network_table), new_label_bold("Forget"),          3, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(window->known_network_table), new_label_bold("Last connection"), 4, 0, 1, 1);

    gtk_grid_set_column_spacing(GTK_GRID(window->known_network_table), 10);
    gtk_grid_set_row_spacing(GTK_GRID(window->known_network_table), 10);

    gtk_widget_show_all(window->window);
    add_all_dbus_objects(window);
    g_signal_connect_swapped(window->window, "destroy", G_CALLBACK(window_rm), window);
}

void window_rm(Window *window) {
    int i;

    for (i = 0; i < n_object_types; i ++) {
	while (window->objects[i] != NULL) {
	    object_methods[i].rm(window, window->objects[i]->data);

	    {
		ObjectList *rm;

		rm = window->objects[i];
		window->objects[i] = window->objects[i]->next;
		free(rm);
	    }
	}
    }

    free(window);
    global.window = NULL;
}

void known_network_table_show(GtkToggleButton *button, Window *window) {
    if (gtk_toggle_button_get_active(button)) {
	bin_empty(GTK_BIN(window->main));
	gtk_container_add(GTK_CONTAINER(window->main), window->known_network_table);
    }
}

/*
 * window=NULL implies that all interfaces are to be added to the indicator list, rather
 * than to a window.
 */
void add_all_dbus_objects(Window *window) {
    GList *object_list, *i;

    object_list = g_dbus_object_manager_get_objects(global.manager);
    for (i = object_list; i != NULL; i = i->next) {
	GDBusObject *object;

	object = (GDBusObject *) i->data;
	object_iterate_interfaces(NULL, object, window, interface_add);
	g_object_unref(object);
    }
    g_list_free(object_list);
}

void object_add(GDBusObjectManager *manager, GDBusObject *object, Window *window) {
    object_iterate_interfaces(manager, object, window, interface_add);
}

void object_rm(GDBusObjectManager *manager, GDBusObject *object, Window *window) {
    object_iterate_interfaces(manager, object, window, interface_rm);
}

void object_iterate_interfaces(GDBusObjectManager *manager, GDBusObject *object, Window *window, ObjectIterFunction method) {
	GList *interface_list, *j;

	interface_list = g_dbus_object_get_interfaces(object);
	for (j = interface_list; j != NULL; j = j->next) {
	    GDBusProxy *proxy;

	    proxy = (GDBusProxy *) j->data;
	    method(manager, object, proxy, window);
	    g_object_unref(proxy);
	}
	g_list_free(interface_list);
}

void interface_add(GDBusObjectManager *manager, GDBusObject *object, GDBusProxy *proxy, Window *window) {
    const gchar *name;

    name = g_dbus_proxy_get_interface_name(proxy);

    for (int i = 0; i < n_object_types; i ++) {
	if (strcmp(name, object_methods[i].interface) == 0) {
	    if (window != NULL) {
		// This function has been invoked for a particular window.
		window_add_object(object, proxy, window, i);
	    }
	    else {
		/*
		 * If window=NULL and manager=NULL, then we are only interested in
		 * initializing the indicators.
		 */
		if (manager != NULL && global.window != NULL) {
		    window_add_object(object, proxy, global.window, i);
		}

		/*
		 * There are two scenarios when we want to run this:
		 * (1) due to a new object/interface being added (window=NULL, manager!=NULL)
		 * (2) right after the manager is initialized (window=NULL, manager=NULL
		 */
		if (global.indicators_enable && object_methods[i].indicator_set != NULL) {
		    Indicator **indicator;

		    indicator = &global.indicators;
		    while (*indicator != NULL) {
			indicator = &(*indicator)->next;
		    }
		    *indicator = indicator_new(proxy, object_methods[i].indicator_set);
		}
	    }

	    break;
	}
    }
}

void object_list_append(ObjectList **list, GDBusObject *object, gpointer data) {
    while (*list != NULL) {
	list = &(*list)->next;
    }

    *list = malloc(sizeof(ObjectList));
    (*list)->object = object;
    (*list)->data = data;
    (*list)->next = NULL;
}

void window_add_object(GDBusObject *object, GDBusProxy *proxy, Window *window, int type) {
    gpointer data;

    data = object_methods[type].new(window, object, proxy);
    object_list_append(&window->objects[type], object, data);
}

void interface_rm(GDBusObjectManager *manager, GDBusObject *object, GDBusProxy *proxy, Window *window) {
    const gchar *name;

    name = g_dbus_proxy_get_interface_name(proxy);

    for (int i = 0; i < n_object_types; i ++) {
	if (!strcmp(name, object_methods[i].interface)) {
	    if (global.window != NULL) {
		ObjectList **list;

		list = &global.window->objects[i];
		while ((*list)->object != object) {
		    list = &(*list)->next;
		}

		object_methods[i].rm(global.window, (*list)->data);

		{
		    ObjectList *rm;

		    rm = *list;
		    *list = (*list)->next;
		    free(rm);
		}
	    }
	    break;
	}
    }

    {
	Indicator **indicator;

	indicator = &global.indicators;
	while (*indicator != NULL) {
	    if ((*indicator)->proxy == proxy) {
		Indicator *rm;

		rm = *indicator;
		*indicator = (*indicator)->next;
		indicator_rm(rm);
		break;
	    }
	    indicator = &(*indicator)->next;
	}
    }
}

void couple_register(Window *window, CoupleType couple_type, int this, gpointer data, GDBusObject *object) {
    CoupleList **list;

    list = &window->couples[couple_type];
    while (*list != NULL) {
	if ( (*list)->object == object) {
	    if ( (*list)->data[this] == NULL) {
		(*list)->data[this] = data;
	    }
	    else {
		CoupleList *new_entry;
		new_entry = malloc(sizeof(CoupleList));
		new_entry->object = object;
		new_entry->data[this] = data;
		new_entry->data[1 - this] = (*list)->data[1 - this];

		new_entry->next = (*list)->next;
		*list = new_entry;
	    }
	    couple_methods[couple_type].bind( (*list)->data[0], (*list)->data[1] );
	    return;
	}
	else {
	    list = &(*list)->next;
	}
    }

    {
	CoupleList *new_entry;
	new_entry = malloc(sizeof(CoupleList));
	new_entry->object = object;
	new_entry->data[this] = data;
	new_entry->data[1 - this] = NULL;

	new_entry->next = NULL;
	*list = new_entry;
    }
}

void couple_unregister(Window *window, CoupleType couple_type, int this, gpointer data) {
    CoupleList **list;

    list = &window->couples[couple_type];
    while (*list != NULL) {
	if ( (*list)->data[this] == data ) {
	    if ( (*list)->data[1 - this] == NULL) {
		// Couple node is empty - delete it
		CoupleList *rm;

		rm = *list;
		*list = (*list)->next;
		free(rm);
	    }
	    else {
		// Unbind the couple
		couple_methods[couple_type].unbind( (*list)->data[0], (*list)->data[1] );
		(*list)->data[this] = NULL;
		list = &(*list)->next;
	    }
	}
	else {
	    list = &(*list)->next;
	}
    }
}
