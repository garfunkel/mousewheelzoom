#define XK_LATIN1
#define XK_MISCELLANY
#define XK_XKB_KEYS


#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <gio/gio.h>
#include <X11/Xlib.h>
#include <X11/keysymdef.h>


#define DBUS_NAME_MAGNIFIER "org.gnome.Magnifier"						// DBus magnifier name
#define DBUS_PATH_ZOOMER_0 "/org/gnome/Magnifier/ZoomRegion/zoomer0"	// DBus path to zoomer
#define DBUS_INTERFACE_ZOOM_REGION "org.gnome.Magnifier.ZoomRegion"		// DBus interface for zoom region
#define DBUS_METHOD_GET_MAG_FACTOR "getMagFactor"						// DBus getMagFactor() method
#define DBUS_METHOD_SET_MAG_FACTOR "setMagFactor"						// DBUS setMagFactor() method

#define ZOOM_INCREMENT 1.06				// zoom increment
#define ZOOM_ITERATIONS 4				// number of zoom iterations
#define ZOOM_FACTOR_MIN 1				// minimum zoom factor
#define ZOOM_FACTOR_MAX 32				// maximum zoom factor
#define ZOOM_MODIFIER Mod1Mask			// alt modifier
#define ZOOM_IN_MOUSE Button4			// mouse scroll up
#define ZOOM_OUT_MOUSE Button5			// mouse scroll down
#define ZOOM_IN_KEYBOARD XK_equal		// keyboard equal (also plus)
#define ZOOM_OUT_KEYBOARD XK_minus		// keyboard minus
#define ZOOM_IN_NUMPAD XK_KP_Add		// numpad add
#define ZOOM_OUT_NUMPAD XK_KP_Subtract	// numpad subtract


static const KeySym ZOOM_KEYS[] = {
	ZOOM_IN_KEYBOARD,	// zoom in using keyboard
	ZOOM_OUT_KEYBOARD,	// zoom out using keyboard
	ZOOM_IN_NUMPAD,		// zoom in using numpad
	ZOOM_OUT_NUMPAD		// zoom out using numpad
};
static const KeySym ZOOM_BUTTONS[] = {
	ZOOM_IN_MOUSE,	// zoom in using mouse
	ZOOM_OUT_MOUSE,	// zoom out using mouse
};
static const Mask MASKS[] = {
	NoSymbol,			// no modifiers
	Mod2Mask,			// numlock
	LockMask,			// capslock
	LockMask | Mod2Mask	// capslock + numlock
};
static const struct timespec ZOOM_ITERATION_DELAY = {
	0,		// seconds
	1000	// nanoseconds
};


/* Get current magnification factor. */
static gdouble get_mag_factor(GDBusConnection *connection) {
	GDBusMessage *request = g_dbus_message_new_method_call(
		DBUS_NAME_MAGNIFIER,
		DBUS_PATH_ZOOMER_0,
		DBUS_INTERFACE_ZOOM_REGION,
		DBUS_METHOD_GET_MAG_FACTOR
	);

	g_debug("DBus %s() request message: \n%s", DBUS_METHOD_GET_MAG_FACTOR, g_dbus_message_print(request, 4));

	GError *error = NULL;

	GDBusMessage *reply = g_dbus_connection_send_message_with_reply_sync(
		connection,
		request,
		G_DBUS_SEND_MESSAGE_FLAGS_NONE,
		-1,
		NULL,
		NULL,
		&error
	);

	g_object_unref(request);

	gdouble magFactor = 1;

	if (error) {
		g_warning("unable to get current magnification factor: %s", error->message);
		g_clear_error(&error);
	} else {
		g_debug("DBus %s() reply message: \n%s", DBUS_METHOD_GET_MAG_FACTOR, g_dbus_message_print(reply, 4));

		GVariant *body = g_dbus_message_get_body(reply);

		if (body) {
			g_variant_get_child(body, 0, "d", &magFactor);
		} else {
			g_warning("DBus %s() did not return a message body", DBUS_METHOD_GET_MAG_FACTOR);
		}

		g_object_unref(reply);
	}

	return magFactor;
}

/* Set magnification level. */
static void set_mag_factor(GDBusConnection *connection, gdouble magFactor) {
	GDBusMessage *request = g_dbus_message_new_method_call(
		DBUS_NAME_MAGNIFIER,
		DBUS_PATH_ZOOMER_0,
		DBUS_INTERFACE_ZOOM_REGION,
		DBUS_METHOD_SET_MAG_FACTOR
	);

	g_dbus_message_set_body(request, g_variant_new("(dd)", magFactor, magFactor));

	g_debug("DBus %s() request message: \n%s", DBUS_METHOD_SET_MAG_FACTOR, g_dbus_message_print(request, 4));

	GError *error = NULL;

	GDBusMessage *reply = g_dbus_connection_send_message_with_reply_sync(
		connection,
		request,
		G_DBUS_SEND_MESSAGE_FLAGS_NONE,
		-1,
		NULL,
		NULL,
		&error
	);

	g_object_unref(request);

	if (error) {
		g_warning("unable to set magnification factor: %s", error->message);
		g_clear_error(&error);
	} else {
		g_debug("DBus %s() reply message: \n%s", DBUS_METHOD_SET_MAG_FACTOR, g_dbus_message_print(reply, 4));
		g_object_unref(reply);
	}
}

/* Handler for when DBus name appears. */
static void on_name_appeared(
	GDBusConnection *connection,
	const gchar *name,
	const gchar *name_owner,
	gpointer user_data
) {
	g_debug("DBus name %s appeard", name);

	Display *display = XOpenDisplay(NULL);
	
	if (!display) {
		g_error("could not open display");
	}

	gdouble magFactor = get_mag_factor(connection);

	for (int maskIndex = 0; maskIndex < sizeof(MASKS) / sizeof(MASKS[0]); maskIndex++) {
		for (int zoomKeyIndex = 0; zoomKeyIndex < sizeof(ZOOM_KEYS) / sizeof(ZOOM_KEYS[0]); zoomKeyIndex++) {
			XGrabKey(
				display,
				XKeysymToKeycode(display, ZOOM_KEYS[zoomKeyIndex]),
				ZOOM_MODIFIER | MASKS[maskIndex],
				DefaultRootWindow(display),
				FALSE,
				GrabModeAsync,
				GrabModeAsync
			);
		}

		for (int zoomButtonIndex = 0; zoomButtonIndex < sizeof(ZOOM_BUTTONS) / sizeof(ZOOM_BUTTONS[0]); zoomButtonIndex++) {
			XGrabButton(
				display,
				ZOOM_BUTTONS[zoomButtonIndex],
				ZOOM_MODIFIER | MASKS[maskIndex],
				DefaultRootWindow(display),
				FALSE,
				0,
				GrabModeAsync,
				GrabModeAsync,
				None,
				None
			);
		}
	}

	XEvent event;
	KeySym keySym = NoSymbol;

	while (TRUE) {
		XNextEvent(display, &event);

		if (event.type == KeyPress || event.type == ButtonPress) {
			for (int iteration = 0; iteration < ZOOM_ITERATIONS; iteration++) {
				keySym = XLookupKeysym(&event.xkey, 0);

				if (event.xbutton.button == ZOOM_OUT_MOUSE
						|| keySym == ZOOM_OUT_KEYBOARD
						|| keySym == ZOOM_OUT_NUMPAD) {
					if (magFactor <= ZOOM_FACTOR_MIN) {
						break;
					}

					magFactor *= 1 / ZOOM_INCREMENT;
					magFactor = magFactor < ZOOM_FACTOR_MIN ? ZOOM_FACTOR_MIN : magFactor;
				} else if (event.xbutton.button == ZOOM_IN_MOUSE
						|| keySym == ZOOM_IN_KEYBOARD
						|| keySym == ZOOM_IN_NUMPAD) {
					if (magFactor >= ZOOM_FACTOR_MAX) {
						break;
					}

					magFactor *= ZOOM_INCREMENT / 1;
					magFactor = magFactor > ZOOM_FACTOR_MAX ? ZOOM_FACTOR_MAX : magFactor;
				} else {
					g_warning("XLookupKeysym(): unexpected button/keysym %i/%lu", event.xbutton.button, keySym);
				}

				set_mag_factor(connection, magFactor);
				
				if (nanosleep(&ZOOM_ITERATION_DELAY, NULL)) {
					g_warning("nanosleep(): %s", strerror(errno));

					errno = 0;
				}
			}
		}
	}

	if (XCloseDisplay(display)) {
		g_error("could not close display");
	}
}

/* Handler for when DBus name vanishes. */
static void on_name_vanished(
	GDBusConnection *connection,
	const gchar *name,
	gpointer user_data
) {
	g_debug("DBus name %s vanished", name);
}

int main() {
	guint watcher_id = g_bus_watch_name(
		G_BUS_TYPE_SESSION,
		DBUS_NAME_MAGNIFIER,
		G_BUS_NAME_WATCHER_FLAGS_NONE,
		on_name_appeared,
		on_name_vanished,
		NULL,
		NULL	
	);

	GMainLoop *loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(loop);
	g_main_loop_unref(loop);

	g_bus_unwatch_name(watcher_id);

	return 0;
}
