#include "eSDKOBS.h"
#include <stdio.h>
#include <time.h>
// 响应回调函数，可以在这个回调中把properties的内容记录到callback_data(用户自定义回调数据)中
obs_status response_properties_callback(const obs_response_properties *properties, void *callback_data);
// 结束回调函数，可以在这个回调中把obs_status和obs_error_details的内容记录到callback_data(用户自定义回调数据)中
void response_complete_callback(obs_status status, const obs_error_details *error, void *callback_data);
int main()
{
    // 以下示例展示如何获取桶的存量信息：
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

    // 设置响应回调函数
    obs_response_handler response_handler =
    {
        &response_properties_callback,
        &response_complete_callback
    };
    obs_status  ret_status = OBS_STATUS_BUTT;
    //定义桶容量缓存及对象数缓存
    char capacity[OBS_COMMON_LEN_256 + 1] = { 0 };
    char obj_num[OBS_COMMON_LEN_256 + 1] = { 0 };
    // 获取桶存量信息
    get_bucket_storage_info(&options, OBS_COMMON_LEN_256 + 1, capacity, OBS_COMMON_LEN_256 + 1, obj_num,
        &response_handler, &ret_status);
    // 判断请求是否成功
    if (ret_status == OBS_STATUS_OK) {
        printf("get_bucket_storage_info success,bucket=%s objNum=%s capacity=%s\n",
            bucketName, obj_num, capacity);
    }
    else
    {
        printf("get_bucket_storage_info failed(%s).\n", obs_get_status_name(ret_status));
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
// 结束回调函数，可以在这个回调中把obs_status和obs_error_details的内容记录到callback_data(用户自定义回调数据)中
void response_complete_callback(obs_status status, const obs_error_details *error, void *callback_data)
{
    if (callback_data) {
        obs_status *ret_status = (obs_status *)callback_data;
        *ret_status = status;
    } else {
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
