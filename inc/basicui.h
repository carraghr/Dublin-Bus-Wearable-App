#ifndef __basicui_H__
#define __basicui_H__

#include <app.h>
#include <Elementary.h>
#include <system_settings.h>
#include <efl_extension.h>
#include <dlog.h>

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "basicui"

#if !defined(PACKAGE)
#define PACKAGE "org.example.basicui"
#endif

#define ELM_DEMO_EDJ "/opt/usr/apps/org.example.basicui/res/ui_controls.edj"
#define ICON_DIR "/opt/usr/apps/org.example.basicui/res/images"

typedef struct appdata {
	Evas_Object *win;
	Evas_Object *conform;
	Evas_Object *layout;
	Evas_Object *label;
	Evas_Object *nf;
	Evas_Object *datetime;
	Evas_Object *popup;
	Evas_Object *button;
	Eext_Circle_Surface *circle_surface;
	struct tm saved_time;
} appdata_s;

void bus_Stop_entry_cb(void *data, Evas_Object * obj, void *event_info);
#endif /* __basicui_H__ */
