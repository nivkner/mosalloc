#include <sys/mman.h>
#include "ParseCsv.h"
#include "globals.h"
#include <cstdio>

/**
 *
 * @param ls
 * @param path to csv file in structure:_page_size,_start_offset,_end_offset
 * assuming line lenght at most 1024 chars
 *  assuming the configuration of the file
 *
 */
void parseCsv::ParseCsv(PoolConfigurationData& configurationData, const char* path, const char* poolType){
    FILE *fp = fopen(path, "r");
    char buf[1024];
    if (!fp)
        THROW_EXCEPTION("cant open csv file");
    int one_time_size = 0;
    long long int _start_offset, _end_offset, _page_size;
    char *delimiter_token;
    const char *delimiter = ",";
    if(!fgets(buf, 1024, fp))
        THROW_EXCEPTION("cant read headers");

    while (fgets(buf, 1024, fp)) {
        delimiter_token = strtok(buf, delimiter);
        if(strcmp(delimiter_token, poolType)){
            continue;
        }

        delimiter_token = strtok(nullptr, delimiter);
        _page_size = atoll(delimiter_token);
        if( _page_size!=-1 && _page_size!= static_cast<size_t>(PageSize::HUGE_1GB) && _page_size!= static_cast<size_t>(PageSize::HUGE_2MB)){
            THROW_EXCEPTION("unknown page size");
        }

        if(_page_size == -1 ){
            if(!one_time_size){
                one_time_size = 1;
            }
            else THROW_EXCEPTION("pool size already exist");
        }

        delimiter_token = strtok(nullptr, delimiter);
        _start_offset = atoll(delimiter_token);
        if(_start_offset < 0 )
            THROW_EXCEPTION("start offset negative");
        delimiter_token = strtok(nullptr, delimiter);
        _end_offset = atoll(delimiter_token);
        if(_end_offset < 0)
            THROW_EXCEPTION("end offset negative");
        delimiter_token = strtok(nullptr, delimiter);
        if(delimiter_token)
            THROW_EXCEPTION("csv configuration is bad");
        if(_page_size !=-1) configurationData.intervalList.AddInterval(_start_offset, _end_offset, (PageSize)_page_size);
        else configurationData.size= _end_offset - _start_offset;
    }
    configurationData.intervalList.Sort();
    fclose(fp);
}