/*
 * Copyright (C) 2017 Richard Hughes <richard@hughsie.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#define G_LOG_DOMAIN				"FuDeviceLocker"

#include "config.h"

#include <gio/gio.h>
#ifdef HAVE_GUSB
#include <gusb.h>
#endif

#include "fu-device-locker.h"
#include "fu-usb-device.h"

/**
 * SECTION:fu-device-locker
 * @title: FuDeviceLocker
 * @short_description: a device helper object
 *
 * An object that makes it easy to close a device when an object goes out of
 * scope.
 *
 * See also: #FuDevice
 */

struct _FuDeviceLocker {
	GObject			 parent_instance;
	GObject			*device;
	gboolean		 device_open;
	FuDeviceLockerFunc	 open_func;
	FuDeviceLockerFunc	 close_func;
};

G_DEFINE_TYPE (FuDeviceLocker, fu_device_locker, G_TYPE_OBJECT)

static void
fu_device_locker_finalize (GObject *obj)
{
	FuDeviceLocker *self = FU_DEVICE_LOCKER (obj);

	/* close device */
	if (self->device_open) {
		g_autoptr(GError) error = NULL;
		if (!self->close_func (self->device, &error))
			g_warning ("failed to close device: %s", error->message);
	}
	if (self->device != NULL)
		g_object_unref (self->device);
	G_OBJECT_CLASS (fu_device_locker_parent_class)->finalize (obj);
}

static void
fu_device_locker_class_init (FuDeviceLockerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = fu_device_locker_finalize;
}

static void
fu_device_locker_init (FuDeviceLocker *self)
{
}

/**
 * fu_device_locker_close:
 * @self: a #FuDeviceLocker
 * @error: (nullable): optional return location for an error
 *
 * Closes the locker before it gets cleaned up.
 *
 * This function can be used to manually close a device managed by a locker,
 * and allows the caller to properly handle the error.
 *
 * Returns: %TRUE for success
 *
 * Since: 1.4.0
 **/
gboolean
fu_device_locker_close (FuDeviceLocker *self, GError **error)
{
	g_autoptr(GError) error_local = NULL;
	g_return_val_if_fail (FU_IS_DEVICE_LOCKER (self), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
	if (!self->device_open)
		return TRUE;
	if (!self->close_func (self->device, &error_local)) {
#ifdef HAVE_GUSB
		if (G_USB_IS_DEVICE (self->device) &&
		    g_error_matches (error_local,
				     G_USB_DEVICE_ERROR,
				     G_USB_DEVICE_ERROR_NO_DEVICE)) {
			g_debug ("ignoring: %s", error_local->message);
			return TRUE;
		} else {
			g_propagate_error (error, g_steal_pointer (&error_local));
			return FALSE;
		}
#else
		g_propagate_error (error, g_steal_pointer (&error_local));
		return FALSE;
#endif
	}
	self->device_open = FALSE;
	return TRUE;
}

/**
 * fu_device_locker_new:
 * @device: a #GObject
 * @error: (nullable): optional return location for an error
 *
 * Opens the device for use. When the #FuDeviceLocker is deallocated the device
 * will be closed and any error will just be directed to the console.
 * This object is typically called using g_autoptr() but the device can also be
 * manually closed using g_clear_object().
 *
 * The functions used for opening and closing the device are set automatically.
 * If the @device is not a type or supertype of #GUsbDevice or #FuDevice then
 * this function will not work.
 *
 * For custom objects please use fu_device_locker_new_full().
 *
 * NOTE: If the @open_func failed then the @close_func will not be called.
 *
 * Think of this object as the device ownership.
 *
 * Returns: a device locker, or %NULL if the @open_func failed.
 *
 * Since: 1.0.0
 **/
FuDeviceLocker *
fu_device_locker_new (gpointer device, GError **error)
{
	g_return_val_if_fail (device != NULL, NULL);
	g_return_val_if_fail (error != NULL, NULL);

#ifdef HAVE_GUSB
	/* GUsbDevice */
	if (G_USB_IS_DEVICE (device)) {
		return fu_device_locker_new_full (device,
						  (FuDeviceLockerFunc) g_usb_device_open,
						  (FuDeviceLockerFunc) g_usb_device_close,
						  error);
	}
#endif

	/* FuDevice */
	if (FU_IS_DEVICE (device)) {
		return fu_device_locker_new_full (device,
						  (FuDeviceLockerFunc) fu_device_open,
						  (FuDeviceLockerFunc) fu_device_close,
						  error);
	}
	g_set_error_literal (error,
			     G_IO_ERROR,
			     G_IO_ERROR_NOT_SUPPORTED,
			     "device object type not supported");
	return NULL;
}

/**
 * fu_device_locker_new_full:
 * @device: a #GObject
 * @open_func: (scope async): a function to open the device
 * @close_func: (scope async): a function to close the device
 * @error: (nullable): optional return location for an error
 *
 * Opens the device for use. When the #FuDeviceLocker is deallocated the device
 * will be closed and any error will just be directed to the console.
 * This object is typically called using g_autoptr() but the device can also be
 * manually closed using g_clear_object().
 *
 * NOTE: If the @open_func failed then the @close_func will not be called.
 *
 * Think of this object as the device ownership.
 *
 * Returns: a device locker, or %NULL if the @open_func failed.
 *
 * Since: 1.0.0
 **/
FuDeviceLocker *
fu_device_locker_new_full (gpointer device,
			   FuDeviceLockerFunc open_func,
			   FuDeviceLockerFunc close_func,
			   GError **error)
{
	g_autoptr(FuDeviceLocker) self = NULL;

	g_return_val_if_fail (device != NULL, NULL);
	g_return_val_if_fail (open_func != NULL, NULL);
	g_return_val_if_fail (close_func != NULL, NULL);
	g_return_val_if_fail (error != NULL, NULL);

	/* create object */
	self = g_object_new (FU_TYPE_DEVICE_LOCKER, NULL);
	self->device = g_object_ref (device);
	self->open_func = open_func;
	self->close_func = close_func;

	/* open device */
	if (!self->open_func (device, error))
		return NULL;

	/* success */
	self->device_open = TRUE;
	return g_steal_pointer (&self);
}
