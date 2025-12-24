#include "eSDKOBS.h"
#include <stdio.h>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
// 响应回调函数，可以在这个回调中把properties的内容记录到callback_data(用户自定义回调数据)中
obs_status response_properties_callback(const obs_response_properties *properties, void *callback_data);
int put_file_data_callback(int buffer_size, char *buffer,
    void *callback_data);
void put_file_complete_callback(obs_status status,
    const obs_error_details *error,
    void *callback_data);
typedef struct put_file_object_callback_data
{
    FILE *infile;
    uint64_t content_length;
    obs_status ret_status;
} put_file_object_callback_data;
uint64_t open_file_and_get_length(char *localfile, put_file_object_callback_data *data);
int main()
{
    // 以下示例展示如何通过put_object上传文件：
    // 在程序入口调用obs_initialize方法来初始化网络、内存等全局资源。
    obs_initialize(OBS_INIT_ALL);
    obs_options options;
    // 创建并初始化options，该参数包括访问域名(host_name)、访问密钥（access_key_id和acces_key_secret）、桶名(bucket_name)、桶存储类别(storage_class)等配置信息
    init_obs_options(&options);
    // host_name填写桶所在的endpoint, 此处以华北-北京四为例，其他地区请按实际情况填写。
    options.bucket_options.host_name = "obs.cn-north-4.myhuaweicloud.com";
    // 认证用的ak和sk硬编码到代码中或者明文存储都有很大的安全风险，建议在配置文件或者环境变量中密文存放，使用时解密，确保安全；
    // 本示例以ak和sk保存在环境变量中为例，运行本示例前请先在本地环境中设置环境变量ACCESS_KEY_ID和SECRET_ACCESS_KEY。
    options.bucket_options.access_key = getenv("ACCESS_KEY_ID");
    options.bucket_options.secret_access_key = getenv("SECRET_ACCESS_KEY");
    // 填写Bucket名称，例如example-bucket-name。
    char * bucketName = "example-bucket-name";
    options.bucket_options.bucket_name = bucketName;
    // 初始化上传对象属性
    obs_put_properties put_properties;
    init_put_properties(&put_properties);
    // 上传对象名
    char *key = "example_put_file_test";
    // 上传的文件
    char file_name[256] = "./example_local_file_test.txt";
    uint64_t content_length = 0;
    // 初始化存储上传数据的结构体
    put_file_object_callback_data data;
    memset(&data, 0, sizeof(put_file_object_callback_data));
    // 打开文件，并获取文件长度
    content_length = open_file_and_get_length(file_name, &data);
    // 设置回调函数
    obs_put_object_handler putobjectHandler =
    {
        { &response_properties_callback, &put_file_complete_callback },
        &put_file_data_callback
    };
    put_object(&options, key, content_length, &put_properties, 0, &putobjectHandler, &data);
    if (OBS_STATUS_OK == data.ret_status) {
        printf("put object from file successfully. \n");
    }
    else
    {
        printf("put object failed(%s).\n",
            obs_get_status_name(data.ret_status));
    }
    if (data.infile != NULL) {
        fclose(data.infile);
    }
    // 释放分配的全局资源
    obs_deinitialize();
}
// 响应回调函数，可以在这个回调中把properties的内容记录到callback_data(用户自定义回调数据)中
obs_status response_properties_callback(const obs_response_properties *properties, void *callback_data)
{
    if (properties == NULL)
    {
        printf("error! obs_response_properties is null!");
        if (callback_data != NULL)
        {
            obs_sever_callback_data *data = (obs_sever_callback_data *)callback_data;
            printf("server_callback buf is %s, len is %llu",
                data->buffer, data->buffer_len);
            return OBS_STATUS_OK;
        }
        else {
            printf("error! obs_sever_callback_data is null!");
            return OBS_STATUS_OK;
        }
    }
    // 打印响应信息
#define print_nonnull(name, field)                                 \
    do {                                                           \
        if (properties-> field) {                                  \
            printf("%s: %s\n", name, properties->field);          \
        }                                                          \
    } while (0)
    print_nonnull("request_id", request_id);
    print_nonnull("request_id2", request_id2);
    print_nonnull("content_type", content_type);
    if (properties->content_length) {
        printf("content_length: %llu\n", properties->content_length);
    }
    print_nonnull("server", server);
    print_nonnull("ETag", etag);
    print_nonnull("expiration", expiration);
    print_nonnull("website_redirect_location", website_redirect_location);
    print_nonnull("version_id", version_id);
    print_nonnull("allow_origin", allow_origin);
    print_nonnull("allow_headers", allow_headers);
    print_nonnull("max_age", max_age);
    print_nonnull("allow_methods", allow_methods);
    print_nonnull("expose_headers", expose_headers);
    print_nonnull("storage_class", storage_class);
    print_nonnull("server_side_encryption", server_side_encryption);
    print_nonnull("kms_key_id", kms_key_id);
    print_nonnull("customer_algorithm", customer_algorithm);
    print_nonnull("customer_key_md5", customer_key_md5);
    print_nonnull("bucket_location", bucket_location);
    print_nonnull("obs_version", obs_version);
    print_nonnull("restore", restore);
    print_nonnull("obs_object_type", obs_object_type);
    print_nonnull("obs_next_append_position", obs_next_append_position);
    print_nonnull("obs_head_epid", obs_head_epid);
    print_nonnull("reserved_indicator", reserved_indicator);
    int i;
    for (i = 0; i < properties->meta_data_count; i++) {
        printf("x-obs-meta-%s: %s\n", properties->meta_data[i].name,
            properties->meta_data[i].value);
    }
    return OBS_STATUS_OK;
}
int put_file_data_callback(int buffer_size, char *buffer,
    void *callback_data)
{
    put_file_object_callback_data *data =
        (put_file_object_callback_data *)callback_data;
    int ret = 0;
    if (data->content_length) {
        int toRead = ((data->content_length > (unsigned)buffer_size) ?
            (unsigned)buffer_size : data->content_length);
        ret = fread(buffer, 1, toRead, data->infile);
    }
    uint64_t originalContentLength = data->content_length;
    data->content_length -= ret;
    if (data->content_length) {
        printf("%llu bytes remaining ", (unsigned long long)data->content_length);
        printf("(%d%% complete) ...\n",
            (int)(((originalContentLength - data->content_length) * 100) / originalContentLength));
    }
    return ret;
}
void put_file_complete_callback(obs_status status,
    const obs_error_details *error,
    void *callback_data)
{
    put_file_object_callback_data *data = (put_file_object_callback_data *)callback_data;
    data->ret_status = status;
}
uint64_t open_file_and_get_length(char *localfile, put_file_object_callback_data *data)
{
    uint64_t content_length = 0;
    const char *body = 0;
    if (!content_length)
    {
        struct stat statbuf;
        if (stat(localfile, &statbuf) == -1)
        {
            fprintf(stderr, "\nERROR: Failed to stat file %s: ",
                localfile);
            return 0;
        }
        content_length = statbuf.st_size;
    }
    if (!(data->infile = fopen(localfile, "rb")))
    {
        fprintf(stderr, "\nERROR: Failed to open input file %s: ",
            localfile);
        return 0;
    }
    data->content_length = content_length;
    return content_length;
}
