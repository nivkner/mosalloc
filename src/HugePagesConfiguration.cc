#include <iostream>
#include <cstring>
#include "HugePagesConfiguration.h"
#include "globals.h"

HugePagesConfiguration::HugePagesConfiguration() {
    ReadBrkPoolEnvParams(_brk_pool_params);
    ReadMmapPoolEnvParams(_mmap_pool_params);
    ReadFileBackedPoolEnvParams(_file_backed_pool_params);
    ReadGeneralEnvParams(_general_params);
}

HugePagesConfiguration::HugePagesConfigurationParams &
HugePagesConfiguration::ReadFromEnvironmentVariables(
        HugePagesConfiguration::ConfigType type) {
    switch (type) {
        case ConfigType::BRK_POOL:
            return _brk_pool_params;
        case ConfigType::MMAP_POOL:
            return _mmap_pool_params;
        case ConfigType::FILE_BACKED_POOL:
            return _file_backed_pool_params;
        default:
            THROW_EXCEPTION("invalid type!");
    }
}

HugePagesConfiguration::GeneralParams &
HugePagesConfiguration::GetGeneralParams() {
    return _general_params; 
}

unsigned long HugePagesConfiguration::GetEnvironmentVariableValue(
        const char *key) const {
    char *val = getenv(key);
    if (val == NULL) {
        THROW_EXCEPTION("environment key was not found");
    }
    return stoul(val);
}

void HugePagesConfiguration::ReadGeneralEnvParams(
        HugePagesConfiguration::GeneralParams &params) {
    char *analyze_val = getenv(ANALYZE_HPBRS_ENV_VAR);
    params._analyze_hpbrs = (analyze_val == NULL) ? false 
        : (stoul(analyze_val) != 0);
    
    char *verbose_val = getenv(VERBOSE_LEVEL_ENV_VAR);
    params._verbose_level = (verbose_val == NULL) ? 0 : stoul(verbose_val);
}

void HugePagesConfiguration::ReadMmapPoolEnvParams(
        HugePagesConfiguration::HugePagesConfigurationParams &params) {

    params._1gb_start = GetEnvironmentVariableValue(
            MMAP_1GB_START_OFFSET_ENV_VAR);
    params._1gb_end = GetEnvironmentVariableValue(MMAP_1GB_END_OFFSET_ENV_VAR);
    params._2mb_start = GetEnvironmentVariableValue(
            MMAP_2MB_START_OFFSET_ENV_VAR);
    params._2mb_end = GetEnvironmentVariableValue(MMAP_2MB_END_OFFSET_ENV_VAR);
    params._pool_size = GetEnvironmentVariableValue(MMAP_POOL_SIZE_ENV_VAR);
    params._ffa_list_size = GetEnvironmentVariableValue(MMAP_FFA_SIZE_ENV_VAR);

    size_t max_end_offset = params._1gb_end > params._2mb_end ?
                            params._1gb_end : params._2mb_end;
    if (max_end_offset > params._pool_size) {
        THROW_EXCEPTION("mmap pool size does not match given offsets");
    }
}

void HugePagesConfiguration::ReadBrkPoolEnvParams(
        HugePagesConfiguration::HugePagesConfigurationParams &params) {

    params._1gb_start = GetEnvironmentVariableValue(
            BRK_1GB_START_OFFSET_ENV_VAR);
    params._1gb_end = GetEnvironmentVariableValue(BRK_1GB_END_OFFSET_ENV_VAR);
    params._2mb_start = GetEnvironmentVariableValue(
            BRK_2MB_START_OFFSET_ENV_VAR);
    params._2mb_end = GetEnvironmentVariableValue(BRK_2MB_END_OFFSET_ENV_VAR);
    params._pool_size = GetEnvironmentVariableValue(BRK_POOL_SIZE_ENV_VAR);
    params._ffa_list_size = 0;

    size_t max_end_offset = params._1gb_end > params._2mb_end ?
                            params._1gb_end : params._2mb_end;
    if (max_end_offset > params._pool_size) {
        THROW_EXCEPTION("brk pool size does not match given offsets");
    }
}

void HugePagesConfiguration::ReadFileBackedPoolEnvParams(
        HugePagesConfiguration::HugePagesConfigurationParams &params) {

    params._1gb_start = 0;
    params._1gb_end = 0;
    params._2mb_start = 0;
    params._2mb_end = 0;
    params._pool_size = GetEnvironmentVariableValue(
            FILE_BACKED_POOL_SIZE_ENV_VAR);
    params._ffa_list_size = GetEnvironmentVariableValue(
            FILE_BACKED_FFA_SIZE_ENV_VAR);
}

