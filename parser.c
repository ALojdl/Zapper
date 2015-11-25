#include <stdio.h>
#include <string.h>

#include "parser.h"

void getConfiguration(const char *path, init_data_t *data)
{
    FILE *config_file;
    char tmp[20];
    char * pch;
    
    config_file = fopen(path, "r");
    if (config_file == NULL) 
    {
        printf("%s\t: ERROR -> Error opening file!", __func__);
        fflush(stdout);
        return;
    }

    //  [dvb-t]      
    fscanf(config_file, "%s", tmp);
    
    //  frequency=470
    fscanf(config_file, "%s", tmp);    
    pch = strtok (tmp,"=");
    pch = strtok (NULL, "=");
    sscanf(pch, "%d", &data->freq);
    
    //  bandwith=8
    fscanf(config_file, "%s", tmp);    
    pch = strtok (tmp,"=");
    pch = strtok (NULL, "=");
    sscanf(pch, "%d", &data->band);
    
    //  module=DVB-T
    fscanf(config_file, "%s", tmp);    
    pch = strtok (tmp,"=");
    pch = strtok (NULL, "=");
    sscanf(pch, "%s", data->module);
    
    //  [initial_channel]
    fscanf(config_file, "%s", tmp); 
    
    //  aPID=101
    fscanf(config_file, "%s", tmp);    
    pch = strtok (tmp,"=");
    pch = strtok (NULL, "=");
    sscanf(pch, "%d", &data->aPID);   
    
    //  vPID=102
    fscanf(config_file, "%s", tmp);    
    pch = strtok (tmp,"=");
    pch = strtok (NULL, "=");
    sscanf(pch, "%d", &data->vPID);  
    
    //  aType=ac3
    fscanf(config_file, "%s", tmp);    
    pch = strtok (tmp,"=");
    pch = strtok (NULL, "=");
    sscanf(pch, "%s", data->aType);
    
    //  vType=mpeg2
    fscanf(config_file, "%s", tmp);    
    pch = strtok (tmp,"=");
    pch = strtok (NULL, "=");
    sscanf(pch, "%s", data->vType); 
    
    fclose(config_file);
}    
