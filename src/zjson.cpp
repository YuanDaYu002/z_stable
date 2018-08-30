
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#include "json.h"
#include "zjson.h"
#include "plog.h"

struct json_object* find_obj(struct json_object* obj, const char* obj_name)
{
  if(obj && json_object_get_type(obj) == json_type_object)
  {
      json_object_object_foreach(obj, key ,val)
      {
          //printf("%s\t%s\n", key, json_object_get_string(val));
          if(strcmp(key, obj_name)==0) return val;
      }
  }
  return NULL;
}
int json_object_parser(const char* json_body, const char* fmt, ...)
{
    va_list ap;
    int *d;
    unsigned int *u;
    char c, *s;
    float *f;
    char arg_name[32];
    char length[32];
    int  bufsiz ;
    
    memset(arg_name, 0, sizeof(arg_name));

    struct json_object* new_obj = json_tokener_parse(json_body);
    struct json_object* obj;

    if(!new_obj) return -1;

    va_start(ap, fmt);
    while (*fmt)
    {
        c = *fmt;
        if(*fmt++ == '%')
        {
            /* get buffer length*/
            memset(length, 0, sizeof(length));
            while(*fmt >= '0' && *fmt <= '9')
            {
                length[strlen(length)] = *fmt;
                fmt++;
            }
            bufsiz = atoi(length);
            obj = find_obj(new_obj, arg_name);
            if(obj)
            {
                switch (*fmt++) 
                {
                    case 's':              /* string */
                    case 'a': //array
                    case 'o': //object
                        //printf("arg_name:%s\n", arg_name);
                        s = va_arg(ap, char *);
                        //printf("%s\n", s);
                        strncpy(s,json_object_get_string(obj), bufsiz);
                        s[bufsiz-1]='\0';
                        if(bufsiz == 0) fprintf(stderr, "%s:bufsiz is 0!!!!\n", __FUNCTION__);
                        //printf("string %s:%s\n", arg_name, s);
                        break;
                    case 'd':              /* int */
                        d = va_arg(ap, int *);
                        *d = json_object_get_int(obj);
                        //printf("int %s:%d\n", arg_name, *d);
                        break;
                    case 'f':
                        f = va_arg(ap, float *);
                        *f = json_object_get_double(obj);
                        break;
                        //printf("int %s:%f\n", arg_name, *f);
                    case 'u':
                        u = va_arg(ap, unsigned int *);
                        *u = json_object_get_int64(obj);
                        break;
                    default:
                        break;
                }

            }
            memset(arg_name, 0, sizeof(arg_name));
        }
        else
        {
            size_t len = strlen(arg_name);
            if(len >= sizeof(arg_name))
                continue;
            arg_name[len] = c;
        }
    }
    va_end(ap);
    json_object_put(new_obj);
    return 0;
}
char* json_build_objects(const char* fmt, ...)
{
  struct json_object* new_obj = json_object_new_object();
  va_list ap;
  int d;
  unsigned int u;
  char c,*s;
  double f;
  char arg_name[32];

  memset(arg_name, 0, sizeof(arg_name));

  va_start(ap, fmt);
  while (*fmt)
  {
      c = *fmt;
      if(*fmt++ == '%')
      {
          switch (*fmt++) 
          {
              case 's':              /* string */
                  s = va_arg(ap, char *);
                  json_object_object_add(new_obj, arg_name, json_object_new_string(s));
                  break;
              case 'd':              /* int */
                  d = va_arg(ap, int);
                  json_object_object_add(new_obj, arg_name, json_object_new_int(d));
                  break;
              case 'f':
                  f = (double)va_arg(ap, double);
                  json_object_object_add(new_obj, arg_name, json_object_new_double(f));
                  break;
              case 'u':
                  u = va_arg(ap, unsigned int);
                  json_object_object_add(new_obj, arg_name, json_object_new_int64(u));
                  break;
              case 'o'://object
              case 'a'://array
                  s = va_arg(ap, char *);
                  //printf("object:%s\n", s);
                  json_object_object_add(new_obj, arg_name, json_tokener_parse(s));
                  break;
              default:
                  break;
          }
          memset(arg_name, 0, sizeof(arg_name));
      }
      else
      {
          size_t len = strlen(arg_name);
          if(len >= sizeof(arg_name))
              continue;
          arg_name[len] = c;
      }
  }
  va_end(ap);

  char* str = strdup(json_object_get_string(new_obj));
  json_object_put(new_obj);
  return str;
}
int json_array_get_length(const char* array_body)
{
    int len = 0;
    struct json_object* obj = json_tokener_parse(array_body);

    if(obj)
    {
        len = json_object_array_length(obj);
    }
    json_object_put(obj);
    return len;
}
int json_array_get_int_by_idx(const char* array_body, int idx)
{
    int i = 0;
    int is_get = 0;
    struct json_object* obj = json_tokener_parse(array_body);

    if(obj)
    {
        struct json_object* ret_obj;
        ret_obj = json_object_array_get_idx(obj, idx);   
        if(ret_obj)
        {
            i = json_object_get_int(ret_obj);
            is_get = 1;
        }
    }
    json_object_put(obj);
    if(!is_get)
        fprintf(stderr, "%s:get int failed!!!,return 0 instead!!!\n", __FUNCTION__);
    return i;
}
char *json_array_get_object_by_idx(const char* array_body, int idx)
{
    char* str = NULL;
    struct json_object* obj = json_tokener_parse(array_body);

    if(obj)
    {
        struct json_object* ret_obj;
        ret_obj = json_object_array_get_idx(obj, idx);   
        if(ret_obj)
        {
            str = strdup(json_object_get_string(ret_obj));
        }
    }
    json_object_put(obj);
    return str;

}

JSON_OBJECT json_build_new()
{
  	return json_object_new_object();
}
void  json_build_add_string(JSON_OBJECT json, const char* key, const char* value)
{
	json_object_object_add((struct json_object*)json, key, json_object_new_string(value));
}
void  json_build_add_int(JSON_OBJECT json, const char* key, int value)
{
	json_object_object_add((struct json_object*)json, key, json_object_new_int(value));
}

void  json_build_add_object(JSON_OBJECT json, const char* key, JSON_OBJECT json_object)
{
	json_object_object_add((struct json_object*)json, key, (struct json_object*)json_object);
}

char* json_build_end(JSON_OBJECT json)
{
	char* str = strdup(json_object_get_string((struct json_object*)json));
	json_object_put((struct json_object*)json);	
	return str;
}

char* json_build_get_json(JSON_OBJECT json)
{
	return strdup(json_object_get_string((struct json_object*)json));
}

JSON_OBJECT json_build_get_object(JSON_OBJECT json)
{
    return (JSON_OBJECT)json_object_get((struct json_object*)json);
}

void json_build_destroy(JSON_OBJECT json)
{
	json_object_put((struct json_object*)json);
}

JSON_ARRAY json_build_new_array()
{
	return json_object_new_array();
}
void  json_build_array_add(JSON_ARRAY json, JSON_OBJECT json_object)
{
	json_object_array_add((struct json_object*)json, (struct json_object*)json_object);
}

int  json_is_json(const char* json)
{
    if(json == NULL || strlen(json) == 0)
        return 0;
    return json[0] == '{';
}

JSON_PARSER json_parser_new(const char* json)
{ 
	 return json_tokener_parse(json);
}

int  json_parser_has_key(JSON_PARSER json_obj, const char* name)
{
	return find_obj((struct json_object*)json_obj, name) != NULL;
}

char* json_parser_get_string(JSON_PARSER json_obj, const char* name, char* value, 
	int bufsize)
{	
	strcpy(value, "null");
	
	struct json_object* obj = find_obj((struct json_object*)json_obj, name);
	if(obj)
	{
		const char* v = json_object_get_string(obj);

		if(strlen(v) >= bufsize)
		{
			plog("buffer is not enough for %s\n", name);
			return value;
		}
		strcpy(value, v);
	}	
	else
	{
		plog("no found [%s]!\n", name);
	}
	return value;
}

const char* json_parser_get_string_const(JSON_PARSER json_obj, const char* name)
{		
	struct json_object* obj = find_obj((struct json_object*)json_obj, name);
	if(obj)
	{
		return json_object_get_string(obj);
	}	

	plog("no found [%s]!\n", name);

	return NULL;
}


int json_parser_get_string_len(JSON_PARSER json_obj, const char* name)
{		
	struct json_object* obj = find_obj((struct json_object*)json_obj, name);
	if(obj)
	{
		return json_object_get_string_len(obj);
	}	
	plog("no found [%s]!\n", name);
	return 0;
}

JSON_OBJECT json_parser_get_object(JSON_PARSER json_obj, const char* name)
{	
	struct json_object* obj = find_obj((struct json_object*)json_obj, name);
	
	if(obj)	return obj;
	
	plog("no found [%s]!\n", name);

	return NULL;
}

void json_parser_free(JSON_PARSER json_obj)
{
	json_object_put((struct json_object*)json_obj);
}

int  json_parser_get_array_size(JSON_PARSER json_obj)
{
    return json_object_array_length((struct json_object*)json_obj);
}

JSON_OBJECT  json_parser_array_get_object_by_idx(JSON_PARSER json_obj, int idx)
{
    return json_object_array_get_idx((struct json_object*)json_obj, idx);   
}

