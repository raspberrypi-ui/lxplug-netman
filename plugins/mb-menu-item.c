// SPDX-License-Identifier: GPL-2.0+
/* ap-menu-item.c - Class to represent a Wifi access point 
 *
 * Jonathan Blandford <jrb@redhat.com>
 * Dan Williams <dcbw@redhat.com>
 *
 * Copyright (C) 2005 - 2010 Red Hat, Inc.
 */

#include "nm-default.h"

#include <stdio.h>
#include <string.h>

#include "mb-menu-item.h"

typedef struct {
	GtkWidget *strength;
	GtkWidget *detail;
	GtkWidget *hbox;
	GtkWidget *desc;

	char *desc_string;
	guint32    int_strength;
} NMMbMenuItemPrivate;

G_DEFINE_TYPE_WITH_CODE (NMMbMenuItem, nm_mb_menu_item, GTK_TYPE_MENU_ITEM, G_ADD_PRIVATE (NMMbMenuItem));

#define NM_MB_MENU_ITEM_GET_PRIVATE(o) ((NMMbMenuItemPrivate *) nm_mb_menu_item_get_instance_private ((NMMbMenuItem *) o))

static const char *
get_tech_name (guint32 tech)
{
	switch (tech) {
	case MB_TECH_1XRTT:
		return _("CDMA");
	case MB_TECH_EVDO:
		return _("EVDO");
	case MB_TECH_GSM:
		return _("GSM");
	case MB_TECH_GPRS:
		return _("GPRS");
	case MB_TECH_EDGE:
		return _("EDGE");
	case MB_TECH_UMTS:
		return _("UMTS");
	case MB_TECH_HSDPA:
		return _("HSDPA");
	case MB_TECH_HSUPA:
		return _("HSUPA");
	case MB_TECH_HSPA:
		return _("HSPA");
	case MB_TECH_HSPA_PLUS:
		return _("HSPA+");
	case MB_TECH_LTE:
		return _("LTE");
	default:
		return NULL;
	}
}

static void
update_label (NMMbMenuItem *item, gboolean use_bold)
{
	NMMbMenuItemPrivate *priv = NM_MB_MENU_ITEM_GET_PRIVATE (item);

	gtk_label_set_use_markup (GTK_LABEL (priv->desc), use_bold);
	if (use_bold) {
		char *markup = g_markup_printf_escaped ("<b>%s</b>", priv->desc_string);

		gtk_label_set_markup (GTK_LABEL (priv->desc), markup);
		g_free (markup);
	} else
		gtk_label_set_text (GTK_LABEL (priv->desc), priv->desc_string);
}

GtkWidget *
nm_mb_menu_item_new (const char *connection_name,
                     guint32 strength,
                     const char *provider,
                     gboolean active,
                     guint32 technology,
                     guint32 state,
                     gboolean enabled,
                     NMApplet *applet)
{
	NMMbMenuItem *item;
	NMMbMenuItemPrivate *priv;
	const char *tech_name;

	item = g_object_new (NM_TYPE_MB_MENU_ITEM, NULL);
	g_assert (item);

	priv = NM_MB_MENU_ITEM_GET_PRIVATE (item);
	priv->int_strength = strength;

	tech_name = get_tech_name (technology);

	/* Construct the description string */
	switch (state) {
	default:
	case MB_STATE_UNKNOWN:
		priv->desc_string = g_strdup (_("not enabled"));
		break;
	case MB_STATE_IDLE:
		if (connection_name)
			priv->desc_string = g_strdup (connection_name);
		else
			priv->desc_string = g_strdup (_("not registered"));
		break;
	case MB_STATE_HOME:
		if (connection_name) {
			if (provider && tech_name)
				priv->desc_string = g_strdup_printf ("%s (%s %s)", connection_name, provider, tech_name);
			else if (provider || tech_name)
				priv->desc_string = g_strdup_printf ("%s (%s)", connection_name, provider ? provider : tech_name);
			else
				priv->desc_string = g_strdup_printf ("%s", connection_name);
		} else {
			if (provider) {
				if (tech_name)
					priv->desc_string = g_strdup_printf ("%s %s", provider, tech_name);
				else
					priv->desc_string = g_strdup_printf ("%s", provider);
			} else {
				if (tech_name)
					priv->desc_string = g_strdup_printf (_("Home network (%s)"), tech_name);
				else
					priv->desc_string = g_strdup_printf (_("Home network"));
			}
		}
		break;
	case MB_STATE_SEARCHING:
		if (connection_name)
			priv->desc_string = g_strdup (connection_name);
		else
			priv->desc_string = g_strdup (_("searching"));
		break;
	case MB_STATE_DENIED:
		priv->desc_string = g_strdup (_("registration denied"));
		break;
	case MB_STATE_ROAMING:
		if (connection_name) {
			if (tech_name)
				priv->desc_string = g_strdup_printf (_("%s (%s roaming)"), connection_name, tech_name);
			else
				priv->desc_string = g_strdup_printf (_("%s (roaming)"), connection_name);
		} else {
			if (provider) {
				if (tech_name)
					priv->desc_string = g_strdup_printf (_("%s (%s roaming)"), provider, tech_name);
				else
					priv->desc_string = g_strdup_printf (_("%s (roaming)"), provider);
			} else {
				if (tech_name)
					priv->desc_string = g_strdup_printf (_("Roaming network (%s)"), tech_name);
				else
					priv->desc_string = g_strdup_printf (_("Roaming network"));
			}
		}
		break;
	}

	update_label (item, (enabled && connection_name && active));

	/* And the strength icon, if we have strength information at all */
	if (enabled && strength) {
		const char *icon_name = mobile_helper_get_quality_icon_name (strength);
		GdkPixbuf *icon = nma_icon_check_and_load (icon_name, applet);

		if (INDICATOR_ENABLED (applet)) {
			/* app_indicator only uses GdkPixbuf */
			gtk_image_set_from_pixbuf (GTK_IMAGE (priv->strength), icon);
		} else {
			int scale = gtk_widget_get_scale_factor (GTK_WIDGET (priv->strength));
			cairo_surface_t *surface;

			surface = gdk_cairo_surface_create_from_pixbuf (icon, scale, NULL);
			gtk_image_set_from_surface (GTK_IMAGE (priv->strength), surface);
			cairo_surface_destroy (surface);
		}
	}

	return GTK_WIDGET (item);
}

/*******************************************************/

static void
nm_mb_menu_item_init (NMMbMenuItem *self)
{
	NMMbMenuItemPrivate *priv = NM_MB_MENU_ITEM_GET_PRIVATE (self);

	priv->hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	priv->desc = gtk_label_new (NULL);
#ifdef LXPANEL_PLUGIN
	gtk_label_set_xalign (GTK_LABEL (priv->desc), 0.0);
	gtk_label_set_yalign (GTK_LABEL (priv->desc), 0.5);
#else
	gtk_misc_set_alignment (GTK_MISC (priv->desc), 0.0, 0.5);
#endif

	gtk_container_add (GTK_CONTAINER (self), priv->hbox);
	gtk_box_pack_start (GTK_BOX (priv->hbox), priv->desc, TRUE, TRUE, 0);

	priv->strength = gtk_image_new ();
	gtk_box_pack_end (GTK_BOX (priv->hbox), priv->strength, FALSE, TRUE, 0);

	gtk_widget_show (priv->desc);
	gtk_widget_show (priv->strength);
	gtk_widget_show (priv->hbox);
}

static void
finalize (GObject *object)
{
	g_free (NM_MB_MENU_ITEM_GET_PRIVATE (object)->desc_string);

	G_OBJECT_CLASS (nm_mb_menu_item_parent_class)->finalize (object);
}

static void
nm_mb_menu_item_class_init (NMMbMenuItemClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	/* virtual methods */
	object_class->finalize = finalize;
}

