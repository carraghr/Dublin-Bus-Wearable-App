#include "basicui.h"
#include <curl/curl.h>
#include <net_connection.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-glib.h>
#include <glib-object.h>
#include <Eina.h>

Elm_Theme *theme;
Evas_Object *button, *layout;
Evas_Object *entry;
Evas_Object *to_del;

typedef struct _item_data{
	int index;
	Elm_Object_Item *item;
} item_data;

char *dd[] = {
		"Search route", "Entry", "Check", "Entryw", "G0enlist", "Image", "PageControl", "Popup", "Progress",
		"Nocontents", "Radio", "Scroller","Map",
		"(Eext) Datetime", "(Eext) Genlist", "(Eext) More Option",
		"(Eext) ProgressBar", "(Eext) Rotary Selector", "(Eext) Scroller", "(Eext) Slider", "(Eext) Spinner",
		NULL
	};

struct MemoryStruct {
	gchar *memory;
	size_t size;
};

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

static void response_cb(void *data, Evas_Object *obj, void *event_info){
	if (to_del) {
		evas_object_del(to_del);
		to_del = NULL;
	}
	evas_object_del(obj);
}

void maxlength_reached(void *data, Evas_Object *obj, void *event_info){

	Evas_Object *popup;

	if (to_del) return;

	popup = elm_popup_add(layout);
	elm_object_style_set(popup, "toast/circle");
	elm_popup_orient_set(popup, ELM_POPUP_ORIENT_BOTTOM);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_popup_timeout_set(popup, 2.0);
	eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, response_cb, NULL);
	evas_object_smart_callback_add(popup, "block,clicked", response_cb, NULL);
	evas_object_smart_callback_add(popup, "timeout", response_cb, NULL);
	elm_object_part_text_set(popup, "elm.text", "Maximum length reached");
	evas_object_show(popup);


	to_del = popup;
}
static void _popup_hide_finished_cb(void *data, Evas_Object *obj, void *event_info){
	if(!obj) return;
	evas_object_del(obj);
}

static void
_popup_hide_cb(void *data, Evas_Object *obj, void *event_info){
	if(!obj) return;
	elm_popup_dismiss(obj);
}

static void _entry_enter_click(void *data, Evas_Object *obj, void *event_info){

	char const * const stopNum = elm_entry_entry_get(entry);

	char* url[200];

	strcpy(url,"https://data.dublinked.ie/cgi-bin/rtpi/realtimebusinformation?stopid=");
	strcat(url, stopNum);
	strcat(url, "&format=json");

	CURL *curl_handle;
	CURLcode res;

	struct MemoryStruct chunk;

	chunk.memory = malloc(1);
	chunk.size = 0;

	curl_global_init(CURL_GLOBAL_ALL);

	/* init the curl session */
	curl_handle = curl_easy_init();

	connection_h connection;
	int conn_err;
	conn_err = connection_create(&connection);
	char *proxy_address;
	conn_err = connection_get_proxy(connection, CONNECTION_ADDRESS_FAMILY_IPV4, &proxy_address);

	if (conn_err == CONNECTION_ERROR_NONE && proxy_address){
		curl_easy_setopt(curl_handle, CURLOPT_PROXY, proxy_address);
	}

	/* specify URL to get */
	curl_easy_setopt(curl_handle, CURLOPT_URL, url);

	/* send all data to this function  */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

	/* we pass our 'chunk' struct to the callback function */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

	/* some servers don't like requests that are made without a user-agent
	field, so we provide one */
	//curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	/* get it! */
	res = curl_easy_perform(curl_handle);

	Evas_Object* popup = elm_popup_add(layout);

	/* check for errors */
	if(res != CURLE_OK) {
		evas_object_hide(popup);
	}else {

		item_data *id;
		int index = 0;

		JsonParser *parser;
		JsonNode *root;
		GError *error;

		error = NULL;

		// Create a new json parser
		parser = json_parser_new();

		// Load from memory
		gboolean result = json_parser_load_from_data(parser,chunk.memory,-1,&error);


		// If result is TRUE then the file was loaded and parsed Successfully.

		if(result){
			root = json_parser_get_root(parser);

			// A new JsonObject variable for loading root object of the json.
			JsonObject *object;

			// Get All the objects in the Node. In this case 2 Object ["data" + "total"].
			object = json_node_get_object(root);
			
			char *errorCode = json_object_get_string_member(object, "errorcode");
			
			char results[500]="";
			char *cur = results, * const end = results + sizeof results;
			
			if(strcmp(errorCode,"1") == 0){
				cur += snprintf (cur, end-cur, "Error: %s. <br> Please Try Again!", json_object_get_string_member(object, "errormessage"));
			}else if(strcmp(errorCode,"4") == 0){
				cur += snprintf (cur, end-cur, "Error: %s. <br> Please Try Again At Later Time!", json_object_get_string_member(object, "errormessage"));
			}else{
				JsonObject *object_member = json_object_get_object_member(object, "results");
				JsonArray *array;
				array = json_object_get_array_member(object,"results");
				int length = json_object_get_int_member(object,"numberofresults");
			
				gint i;
				for (i = 0; i < length; i++){
					JsonNode *node;
					JsonObject *object;
					const gchar *package_name;
					gint64 ratings_total;

					node = json_array_get_element(array, i);

					/* Get the package name and number of ratings from the object - skip if has no name*/

					object = json_node_get_object (node);

					char* route = json_object_get_string_member(object, "route");
					char* time = json_object_get_string_member(object, "duetime");

					if(cur < end){
						if(strcmp(time,"Due") == 0){
							cur += snprintf (cur, end-cur, "<b>%s</b> \t %s<br>", route, time);
						}else if(strcmp(time,"1") == 0){
							cur += snprintf (cur, end-cur, "<b>%s</b> \t %s Min<br>", route, time);
						}else{
							cur += snprintf (cur, end-cur, "<b>%s</b> \t %s Mins<br>", route, time);
						}
					}
				}
			}
				
			
			elm_object_style_set(popup, "circle");
			evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, _popup_hide_cb, NULL);
			evas_object_smart_callback_add(popup, "dismissed", _popup_hide_finished_cb, NULL);

		    layout = elm_layout_add(popup);
		    elm_layout_theme_set(layout, "layout", "popup", "content/circle");

	    	elm_object_part_text_set(layout, "elm.text", results);
	    	elm_object_content_set(popup, layout);
			evas_object_show(popup);

			//Parsing Successful
			dlog_print(DLOG_DEBUG, "SRBD" ,"SUCCESS on PARSing");

		}else{
			// We Failed!
			dlog_print(DLOG_DEBUG, "SRBD" ,"FAILED to PARSE");
		}
	}

	/* cleanup curl stuff */
	curl_easy_cleanup(curl_handle);

	free(chunk.memory);
	connection_destroy(connection);
	
	/* we're done with libcurl, so clean it up */
	curl_global_cleanup();
}

/* UI function to create entries */
void bus_Stop_entry_cb(void *data, Evas_Object *obj, void *event_info){

	appdata_s *ad = (appdata_s *)data;

	Evas_Object *nf = ad->nf;

	static Elm_Entry_Filter_Limit_Size limit_filter_data;

	layout = elm_layout_add(obj);
	elm_layout_file_set(layout, ELM_DEMO_EDJ, "entry_layout");
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, 0.0);
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, 0.0);

	entry = elm_entry_add(layout);
	elm_entry_single_line_set(entry, EINA_TRUE);
	elm_entry_scrollable_set(entry, EINA_TRUE);
	elm_scroller_policy_set(entry, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
	evas_object_smart_callback_add(entry, "maxlength,reached", maxlength_reached, nf);

	limit_filter_data.max_char_count = 0;
	limit_filter_data.max_byte_count = 100;
	elm_entry_markup_filter_append(entry, elm_entry_filter_limit_size, &limit_filter_data);
	elm_object_part_text_set(entry, "elm.guide", "Enter Stop");
	elm_entry_cursor_end_set(entry);
	evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_smart_callback_add(entry, "activated", _entry_enter_click, NULL);

	elm_object_part_content_set(layout, "entry_part", entry);

	//elm_naviframe_item_push(nf, _("Single line entry"), NULL, NULL, scroller, "empty");
	elm_naviframe_item_push(nf, "EntrpyS", NULL, NULL, layout, "empty");

}
