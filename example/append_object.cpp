#include "eSDKOBS.h"
#include <stdio.h>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
// 响应回调函数，可以在这个回调中把properties的内容记录到callback_data(用户自定义回调数据)中
obs_status response_properties_callback(const obs_response_properties *properties, void *callback_data);
void put_buffer_complete_callback(obs_status status, const obs_error_details *error, void *callback_data);
typedef struct put_buffer_object_callback_data
{
    char *put_buffer;
    uint64_t buffer_size;
    uint64_t cur_offset;
    obs_status ret_status;
} put_buffer_object_callback_data;
int put_buffer_data_callback(int buffer_size, char *buffer, void *callback_data);
char* generate_upload_buffer(uint64_t buffer_size);
int main()
{
    // 以下示例展示如何通过append_object实现追加上传：
    // 在程序入口调用obs_initialize方法来初始化网络、内存等全局资源。
    obs_initialize(OBS_INIT_ALL);
    obs_options options;
    // 创建并初始化options，该参数包括访问域名(host_name)、访问密钥（access_key_id和acces_key_secret）、桶名(bucket_name)、桶存储类别(storage_class)等配置信息
    init_obs_options(&options);
    // host_name填写桶所在的endpoint, 此处以华北-北京四为例，其他地区请按实际情况填写。
    options.bucket_options.host_name = "obs.cn-east-3.myhuaweicloud.com";
    // 认证用的ak和sk硬编码到代码中或者明文存储都有很大的安全风险，建议在配置文件或者环境变量中密文存放，使用时解密，确保安全；
    // 本示例以ak和sk保存在环境变量中为例，运行本示例前请先在本地环境中设置环境变量ACCESS_KEY_ID和SECRET_ACCESS_KEY。
    options.bucket_options.access_key = getenv("ACCESS_KEY_ID");
    options.bucket_options.secret_access_key = getenv("SECRET_ACCESS_KEY");
    // 填写Bucket名称，例如example-bucket-name。
    char * bucketName = "hw-obs-bench";
    options.bucket_options.bucket_name = bucketName;
    // 初始化上传对象属性
    obs_put_properties put_properties;
    init_put_properties(&put_properties);
    //初始化存储上传数据的结构体
    put_buffer_object_callback_data data;
    memset(&data, 0, sizeof(put_buffer_object_callback_data));
    // 设置buffersize
    data.buffer_size = 10 * 1024 * 1024;
    // 创建模拟的流式上传数据buffer, 并赋值到上传数据结构中
    data.put_buffer = generate_upload_buffer(data.buffer_size);
    if (NULL == data.put_buffer) {
        printf("generate put buffer failed. \n");
        return 1;
    }
    // 上传的对象名
    char *key = "example_put_file_test_append";
    // append上传的起始位置
    char *position = "0";
    obs_append_object_handler putobjectHandler =
    {
        { &response_properties_callback, &put_buffer_complete_callback },
          &put_buffer_data_callback
    };
    append_object(&options, key, data.buffer_size, position, &put_properties,
        0, &putobjectHandler, &data);
    if (OBS_STATUS_OK == data.ret_status) {
        printf("append object from buffer successfully. \n");
    }
    else
    {
        printf("append object from buffer failed(%s).\n", obs_get_status_name(data.ret_status));
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
void put_buffer_complete_callback(obs_status status, const obs_error_details *error, void *callback_data)
{
    if (callback_data) {
        put_buffer_object_callback_data *data = (put_buffer_object_callback_data *)callback_data;
        data->ret_status = status;
    }
    else {
        printf("Callback_data is NULL");
    }
    if (error && error->message) {
        printf("Error Message: \n   %s\n", error->message);
    }
    if (error && error->resource) {
        printf("Error Resource: \n  %s\n", error->resource);
    }
    if (error && error->further_details) {
        printf("Error further_details: \n   %s\n", error->further_details);
    }
    if (error && error->extra_details_count) {
        int i;
        for (i = 0; i < error->extra_details_count; i++) {
            printf("Error Extra Detail(%d):\n   %s:%s\n", i, error->extra_details[i].name,
                error->extra_details[i].value);
        }
    }
    if (error && error->error_headers_count) {
        int i;
        for (i = 0; i < error->error_headers_count; i++) {
            const char *errorHeader = error->error_headers[i];
            printf("Error Headers(%d):\n    %s\n", i, errorHeader == NULL ? "NULL Header" : errorHeader);
        }
    }
}
int put_buffer_data_callback(int buffer_size, char *buffer, void *callback_data)
{
    put_buffer_object_callback_data *data =
        (put_buffer_object_callback_data *)callback_data;
    int toRead = 0;
    if (data->buffer_size) {
        toRead = ((data->buffer_size > (unsigned)buffer_size) ?
            (unsigned)buffer_size : data->buffer_size);
        memcpy(buffer, data->put_buffer + data->cur_offset, toRead);
    }
    uint64_t originalContentLength = data->buffer_size;
    data->buffer_size -= toRead;
    data->cur_offset += toRead;
    if (data->buffer_size) {
        printf("%llu bytes remaining ", (unsigned long long)data->buffer_size);
        printf("(%d%% complete) ...\n",
            (int)(((originalContentLength - data->buffer_size) * 100) / originalContentLength));
    }
    return toRead;
}
// 创建模拟的流式上传数据
char* generate_upload_buffer(uint64_t buffer_size) {
    void* upload_buffer = NULL;
    if (buffer_size > 0) {
        upload_buffer = malloc(buffer_size);
        if (upload_buffer != NULL) {
            memset(upload_buffer, 't', buffer_size);
        }
    }
    return (char *)upload_buffer;
}
