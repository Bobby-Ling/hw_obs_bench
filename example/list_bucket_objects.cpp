#include "eSDKOBS.h"
#include <stdio.h>
#include <cstdlib>
#include <cstring>
#include <time.h>
#include <sys/stat.h>
// 响应回调函数，可以在这个回调中把properties的内容记录到callback_data(用户自定义回调数据)中
obs_status response_properties_callback(const obs_response_properties *properties, void *callback_data);
void listobjects_complete_callback(obs_status status,
    const obs_error_details *error,
    void *callback_data);
typedef struct list_object_callback_data
{
    int is_truncated;
    char next_marker[1024];
    int keyCount;
    int allDetails;
    obs_status ret_status;
} list_object_callback_data;
obs_status list_objects_callback(int is_truncated, const char *next_marker,
    int contents_count,
    const obs_list_objects_content *contents,
    int common_prefixes_count,
    const char **common_prefixes,
    void *callback_data);
int main()
{
    // 以下示例展示如何通过函数list_bucket_objects列举出桶里的对象。
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
    // 设置响应回调函数
    obs_list_objects_handler list_bucket_objects_handler =
    {
        { &response_properties_callback, &listobjects_complete_callback },
        &list_objects_callback
    };
    // 用户自定义回调数据
    list_object_callback_data data;
    memset(&data, 0, sizeof(list_object_callback_data));
    data.allDetails = 1;
    const char* prefix = NULL;
    const char* marker = NULL;
    const char* delimiter = "/";
    int maxkeys = 1000;
    // 列举对象
    list_bucket_objects(&options, prefix, marker, delimiter, maxkeys, &list_bucket_objects_handler, &data);
    if (OBS_STATUS_OK == data.ret_status) {
        printf("list bucket objects successfully. \n");
    }
    else
    {
        printf("list bucket objects failed(%s).\n",
            obs_get_status_name(data.ret_status));
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
void print_error_details(const obs_error_details *error) {
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
void listobjects_complete_callback(obs_status status,
    const obs_error_details *error,
    void *callback_data)
{
    if (callback_data)
    {
        list_object_callback_data *data = (list_object_callback_data *)callback_data;
        data->ret_status = status;
    }
    else {
        printf("Callback_data is NULL");
    }
    print_error_details(error);
}
static void printListBucketHeader(int allDetails)
{
    printf("%-50s  %-20s  %-5s",
        "   Key",
        "   Last Modified", "Size");
    if (allDetails) {
        printf("  %-34s  %-64s  %-12s            %-12s",
            "   ETag",
            "   Owner ID",
            "Display Name",
            "StorageClass");
    }
    printf("\n");
    printf("--------------------------------------------------  "
        "--------------------  -----");
    if (allDetails) {
        printf("  ----------------------------------  "
            "-------------------------------------------------"
            "---------------  ------------            ------------");
    }
    printf("\n");
}
obs_status list_objects_callback(int is_truncated, const char *next_marker,
    int contents_count,
    const obs_list_objects_content *contents,
    int common_prefixes_count,
    const char **common_prefixes,
    void *callback_data)
{
    list_object_callback_data *data = (list_object_callback_data *)callback_data;
    data->is_truncated = is_truncated;
    if ((!next_marker || !next_marker[0]) && contents_count) {
        next_marker = contents[contents_count - 1].key;
    }
    if (next_marker) {
        snprintf(data->next_marker, sizeof(data->next_marker), "%s",
            next_marker);
    }
    else {
        data->next_marker[0] = 0;
    }
    if (contents_count && !data->keyCount) {
        printListBucketHeader(data->allDetails);
    }
    int i;
    for (i = 0; i < contents_count; i++) {
        const obs_list_objects_content *content = &(contents[i]);
        char timebuf[256] = { 0 };
        time_t t = (time_t)content->last_modified;
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%dT%H:%M:%SZ",
            gmtime(&t));
        char sizebuf[16] = { 0 };
        if (content->size < 100000) {
            sprintf(sizebuf, "%5llu", (unsigned long long) content->size);
        }
        else if (content->size < (1024 * 1024)) {
            sprintf(sizebuf, "%4lluK",
                ((unsigned long long) content->size) / 1024ULL);
        }
        else if (content->size < (10 * 1024 * 1024)) {
            float f = content->size;
            f /= (1024 * 1024);
            sprintf(sizebuf, "%1.2fM", f);
        }
        else if (content->size < (1024 * 1024 * 1024)) {
            sprintf(sizebuf, "%4lluM",
                ((unsigned long long) content->size) /
                (1024ULL * 1024ULL));
        }
        else {
            float f = (content->size / 1024);
            f /= (1024 * 1024);
            sprintf(sizebuf, "%1.2fG", f);
        }
        printf("%-50s  %s  %s", content->key, timebuf, sizebuf);
        if (data->allDetails) {
            printf("  %-36s  %-64s  %-20s  %-16s  %-16s",
                content->etag,
                content->owner_id ? content->owner_id : "",
                content->owner_display_name ? content->owner_display_name : "",
                content->storage_class ? content->storage_class : "",
                content->type ? content->type : ""
            );
        }
        printf("\n");
    }
    data->keyCount += contents_count;
    for (i = 0; i < common_prefixes_count; i++) {
        printf("\nCommon Prefix: %s\n", common_prefixes[i]);
    }
    printf("contents_count:%d\n", contents_count);
    return OBS_STATUS_OK;
}
