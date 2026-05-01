#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>
#include <stdio.h>

#include "at_command.h"
#include "config_parameter.h"
#include "save_to_nvs.h"

void convert_to_json_update(const char *data) {
    if(data==NULL) return;
    char dt[1024];
    strcpy(dt,data);
    char *final_data=NULL;
    char *start;char *end;
    start = (char *)dt;
    while (1){
      start = strchr(start,'{');
      if(start==NULL) break;   
      end = strchr(start,'}');
      if(end==NULL) break;
      int len = end - start + 2;
      final_data = malloc(len + 1);
      memcpy(final_data,start,len);
      final_data[len]='\0';
      //printf("data_final: %s\r\n",final_data);
    //   if(strstr(final_data,"\"command_type\":208")!=NULL) {
    //     free(final_data);
    //     start=end+1;
    //   }
    //   else {
    printf("data_final: %s\r\n",final_data);
    parse_json(final_data);
    //convert_to_json(final_data);
    start=end+1;
    free(final_data);
    //   }
     }
}

