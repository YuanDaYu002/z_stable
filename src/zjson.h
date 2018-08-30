#ifndef ZJSON_H
#define ZJSON_H

#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef  INVALID_INT
#define  INVALID_INT    0x19820816
#endif

#define JSON_OBJECT void*

#define JSON_ARRAY  JSON_OBJECT

#define JSON_PARSER JSON_OBJECT 


/*
 * parse json objects
 *
 * char buf1[230], buf2[230],c; 
 * int i;
 * int a=0, b=0;
 * int u;
 * float f=0.0f;
 *
 * const char* tt = "{ \"ID\": [{ \"a\":1234, \"b\":5678 , \"f\":11.2312 }, { \"aa\":1234, \"bb\":5678 , \"ff\":11.2312 }] }";
 * json_object_parser(tmp, "ID%20sNum%dFloat%fdata%30o", buf1, &i, &f, buf2);
 * printf("ID=%s,Num=%d,Float=%f,data=%s\n", buf1,i,f, buf2);
 * ID=id12345678,Num=5678,Float=11.232100,data={ "aa": 123, "bb": 333 } 
 *
 * %20s mean buf1's length is 20
 * %30o mean object data will copy to buf2, which's length is 30
 */
int json_object_parser(const char* json_body, const char* fmt, ...);
/*
 * build json objects 
 *
 *char* str = json_build_objects("ID%sNum%dDou%fUn%udata%o", "id1111", 1231123, 333.221, 3999998899, "{\"a\":12,\"b\":32}");
 * return:{ "ID": "id1111", "Num": 1231123, "Dou": 333.221000, "Un": 3999998899, "data": { "a": 12, "b": 32 } }
 *
 * must free return buffer!!!
 */
char* json_build_objects(const char* fmt, ...);

int   json_array_get_length(const char* array_body);
int   json_array_get_int_by_idx(const char* array_body, int idx);
char *json_array_get_object_by_idx(const char* array_body, int idx);


JSON_OBJECT json_build_new();
void  json_build_add_string(JSON_OBJECT json, const char* key, const char* value);
void  json_build_add_int(JSON_OBJECT json, const char* key, int value);

void  json_build_add_object(JSON_OBJECT json, const char* key, JSON_OBJECT json_object);

char* json_build_end(JSON_OBJECT json);

char* json_build_get_json(JSON_OBJECT json);

JSON_OBJECT json_build_get_object(JSON_OBJECT json);

void  json_build_destroy(JSON_OBJECT json);

/* array */
JSON_ARRAY json_build_new_array();
void  json_build_array_add(void* json, JSON_OBJECT json_object);


int   json_is_json(const char* json);

JSON_PARSER  json_parser_new(const char* json);

int   json_parser_has_key(JSON_PARSER json_obj, const char* name);

char* json_parser_get_string(JSON_PARSER json_obj, const char* name, char* value, 
	int bufsize);

const char* json_parser_get_string_const(JSON_PARSER json_obj, const char* name);

int   json_parser_get_string_len(JSON_PARSER json_obj, const char* name);

JSON_OBJECT json_parser_get_object(JSON_PARSER json_obj, const char* name);

void  json_parser_free(JSON_PARSER json_obj);

int   json_parser_get_array_size(JSON_PARSER json_obj);

JSON_OBJECT  json_parser_array_get_object_by_idx(JSON_PARSER json_obj, int idx);


#define JSON_PARSE_INT_VALUE(save_to, json_obj, key) \
	do { \
		char value[128]; \
		if(json_parser_has_key(json_obj, key)) { \
			json_parser_get_string(json_obj, key, value, sizeof(value)); \
			save_to = atoi(value); \
		} \
	}while(0)
#define JSON_PARSE_FLOAT_VALUE(save_to, json_obj, key) \
	do { \
		char value[128]; \
		if(json_parser_has_key(json_obj, key)) { \
			json_parser_get_string(json_obj, key, value, sizeof(value)); \
			save_to = atof(value); \
		} \
	}while(0)
#define JSON_PARSE_STRING_VALUE(save_to, save_size, json_obj, key) \
	do { \
		if(json_obj && json_parser_has_key(json_obj, key)) { \
			json_parser_get_string(json_obj, key, save_to, save_size); \
		} \
	}while(0)
/* return local const string, no need be free */
#define JSON_PARSE_STRING_CONST(save_to, json_obj, key) \
		do { \
			if(json_obj && json_parser_has_key(json_obj, key)) { \
				save_to = json_parser_get_string_const(json_obj, key); \
			} \
		}while(0)
		
/* return string save in a malloc memory , need be free */
#define JSON_PARSE_STRING_MALLOC(save_to, json_obj, key) \
		do { \
			if(json_obj && json_parser_has_key(json_obj, key)) { \
				*save_to = strdup(json_parser_get_string_const(json_obj, key)); \
			} \
		}while(0)

#ifdef __cplusplus
}
class JsonParser
{
public:
  	explicit JsonParser(const char* json):m_json(NULL)
  	{
  		if(json) 
			m_json = json_parser_new(json);
  	}
  	~JsonParser()
	{
		if(m_json) 
			json_parser_free(m_json);
	}
	const char* GetStringConst(const char* key)
	{
		const char* save_to = NULL;
		JSON_PARSE_STRING_CONST(save_to, m_json, key);
		return save_to;
	}
	/*will not change save_to if key no found or bufsize not enough*/
	char* GetStringValue(char* save_to, int bufsize, const char* key)
	{
		JSON_PARSE_STRING_VALUE(save_to, bufsize, m_json, key);
		return save_to;
	}
	/*will not change save_to if key no found  */
	int GetIntValue(int& save_to, const char* key)
	{
		JSON_PARSE_INT_VALUE(save_to, m_json, key);
		return save_to;
	}

    float GetFloatValue(float& save_to, const char* key)
    {
		JSON_PARSE_FLOAT_VALUE(save_to, m_json, key);
		return save_to;
    }

    int GetIntValue(const char* key)
    {
        int v = INVALID_INT;
        return GetIntValue(v, key);
    }
	/*will not change save_to if key no found  */
	unsigned char GetIntValue(unsigned char& save_to, const char* key)
	{
		JSON_PARSE_INT_VALUE(save_to, m_json, key);
		return save_to;
	}
	
	/*will not change save_to if key no found  */
	unsigned short GetIntValue(unsigned short& save_to, const char* key)
	{
		JSON_PARSE_INT_VALUE(save_to, m_json, key);
		return save_to;
	}

	operator JSON_PARSER()
	{
		return m_json;
	}
private:
  	JsonParser();
	JsonParser & operator = (const JsonParser&);
	JsonParser(const JsonParser & );

	JSON_PARSER m_json;
};

/* 位域结构不方便引用传参修改，这里用宏处理 */
#define JsonParser_GetBitfieldValue(save_to, jp, key) \
	JSON_PARSE_INT_VALUE(save_to, jp, key)

class JsonBuilder
{
 public:
     JsonBuilder():m_json(NULL), m_body(NULL), m_flag(0) { m_json = json_build_new(); }
     ~JsonBuilder() {
         if(m_body) free(m_body);
         if(m_json) json_build_destroy(m_json);
     }
     void AddInt(const char* key, int value) {
         if(m_json && key) json_build_add_int(m_json, key, value);
         m_flag = 1;
     }
     void AddString(const char* key, const char* value) {
         if(m_json && key && value) json_build_add_string(m_json, key, value);
         m_flag = 1;
     }
     void AddJson(const char* key, JsonBuilder & js) {
         if(m_json && key && js.GetObject() ) {
             json_build_add_object(m_json, key, json_build_get_object(js.GetObject()));
             m_flag = 1;
         }
     }
     char* GetJson() { 
         if(m_flag == 0 && m_body) 
             return m_body;
         if(m_body) {
             free(m_body);
             m_body = NULL;
         }
         if(m_json) m_body = json_build_get_json(m_json);
         m_flag = 0;
         return m_body;
     }
     int GetSize() {
         if(!m_body)
             m_body = json_build_get_json(m_json);
         return strlen(m_body);
     }
     JSON_OBJECT GetObject() {
         if(m_json) return m_json;
         else return NULL;
     }
 private:
     JSON_OBJECT m_json;
     char* m_body;
     int   m_flag;
};

// Catch bug where variable name is omitted, e.g. JSON_PARSER_NEW_AUTO (json);
//#define JsonParser(x) COMPILE_ASSERT(0, json_parser_decl_missing_var_name)

#endif

#endif /* end of include guard: ZJSON_H */
