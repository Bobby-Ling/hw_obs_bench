#pragma once

#include "config.h"
#include <cstddef>
#include <cstdlib>
#include <eSDKOBS.h>
#include <log.h>
#include <mutex>
#include <string>
#include <string_view>

#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 60
void print_progress(double percentage, bool end = false) {
    int val = (int)(percentage * 100);
    int lpad = (int)(percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;
    printf("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
    if (end) {
        printf("\n");
    }
    fflush(stdout);
}

class HuaweiCloudObs {
  public:
    HuaweiCloudObs() {
        init();
    }

    ~HuaweiCloudObs() {
        // deinit();
    }

    void put_object(const std::string_view &key, const std::string_view &object) const {
        // 初始化存储上传数据的结构体
        put_buffer_object_callback_data data = {
            // 流式上传数据buffer, 并赋值到上传数据结构中
            .put_buffer = object.data(),
            // 设置buffersize
            .buffer_size = object.size(),
        };

        obs_put_object_handler put_object_handler = {
            {&response_properties_callback, &response_complete_callback},
            &put_buffer_data_callback
        };

        ::put_object(
            &base_option,
            (char *)key.data(),
            data.buffer_size,
            const_cast<obs_put_properties *>(&put_properties),
            0,
            &put_object_handler,
            &data
        );
        LOG_ASSERT(OBS_STATUS_OK == data.ret_status, "data.ret_status: {}", obs_get_status_name(data.ret_status));
    }

    std::size_t append_object(const std::string_view &key, const std::string_view &object, std::size_t start_pos) const {
        // 初始化存储上传数据的结构体
        put_buffer_object_callback_data data = {
            // 流式上传数据buffer, 并赋值到上传数据结构中
            .put_buffer = object.data(),
            // 设置buffersize
            .buffer_size = object.size(),
        };
        obs_append_object_handler append_object_handler = {
            {&response_properties_callback, &response_complete_callback},
            &put_buffer_data_callback
        };
        ::append_object(
            &base_option,
            (char *)key.data(),
            data.buffer_size,
            std::to_string(start_pos).c_str(),
            const_cast<obs_put_properties *>(&put_properties),
            0,
            &append_object_handler,
            &data
        );
        LOG_ASSERT(OBS_STATUS_OK == data.ret_status, "data.ret_status: {}", obs_get_status_name(data.ret_status));
        return data.obs_next_append_position;
    }

    // void create_bucket(const std::string *bucket_name) {
    //     obs_response_handler response_handler = {&response_properties_callback, &response_complete_callback};
    //     obs_options options = base_option;
    //     // 设置桶的存储类别，此处以标准存储为例
    //     options.bucket_options.storage_class = OBS_STORAGE_CLASS_STANDARD;
    //     obs_status ret_status = OBS_STATUS_BUTT;
    //     const char *bucket_location = CONFIG::BUCKET_LOCATION.data();
    //     // 创建桶，并设置桶的ACL权限，此处以设置桶为私有为例
    //     ::create_bucket(&options, OBS_CANNED_ACL_PRIVATE, bucket_location, &response_handler, &ret_status);
    //     // 判断请求是否成功
    //     LOG_ASSERT(OBS_STATUS_OK == ret_status, "");
    // }

  private:
    obs_options base_option;

    obs_put_properties put_properties;

    obs_status init() {
        static std::once_flag once;
        static std::atomic<obs_status> status;
        std::call_once(once, [&]() {
            obs_status ret_status = OBS_STATUS_BUTT;
            ret_status = obs_initialize(OBS_INIT_ALL);
            if (OBS_STATUS_OK != ret_status) {
                LOG_FATAL("obs_initialize failed({}).", obs_get_status_name(ret_status));
                // return ret_status;
            }
            status = ret_status;
            // 请不要多次调用obs_initialize和obs_deinitialize，否则会导致程序访问无效的内存

            init_obs_options(&base_option);
            base_option.bucket_options.host_name = const_cast<char *>(CONFIG::ENDPOINT.data());
            base_option.bucket_options.bucket_name = const_cast<char *>(CONFIG::BUCKET_NAME.data());

            // 认证用的ak和sk硬编码到代码中或者明文存储都有很大的安全风险，建议在配置文件或者环境变量中密文存放，使用时解密，确保安全；本示例以ak和sk保存在环境变量中为例，运行本示例前请先在本地环境中设置环境变量ACCESS_KEY_ID和SECRET_ACCESS_KEY。
            // 您可以登录访问管理控制台获取访问密钥AK/SK，获取方式请参见https://support.huaweicloud.com/usermanual-ca/ca_01_0003.html
            base_option.bucket_options.access_key = const_cast<char*>(CONFIG::ACCESS_KEY_ID.data());
            base_option.bucket_options.secret_access_key = const_cast<char*>(CONFIG::SECRET_ACCESS_KEY.data());

            // 初始化上传对象属性
            init_put_properties(&put_properties);
        });
        return status;
    }

    void deinit() {
        static std::once_flag once;
        std::call_once(once, []() {
            obs_deinitialize();
        });
    }

    struct put_buffer_object_callback_data {
        const char *put_buffer;
        uint64_t buffer_size;
        uint64_t cur_offset;
        std::size_t obs_next_append_position;
        obs_status ret_status;
    };

    // 响应回调函数，可以在这个回调中把properties的内容记录到callback_data(用户自定义回调数据)中
    static obs_status response_properties_callback(const obs_response_properties *properties, void *callback_data) {
        if (properties == NULL) {
            printf("error! obs_response_properties is null!");
            if (callback_data != NULL) {
                obs_sever_callback_data *data = (obs_sever_callback_data *)callback_data;
                printf("server_callback buf is %s, len is %llu", data->buffer, data->buffer_len);
                return OBS_STATUS_OK;
            } else {
                printf("error! obs_sever_callback_data is null!");
                return OBS_STATUS_OK;
            }
        }

        if (properties->obs_next_append_position) {
            static_cast<put_buffer_object_callback_data*>(callback_data)->obs_next_append_position = std::atoll(properties->obs_next_append_position);
        }

        // 打印响应信息
#define print_nonnull(name, field)                       \
    do {                                                 \
        if (properties->field) {                         \
            printf("%s: %s\n", name, properties->field); \
        }                                                \
    } while (0)
        /*
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
            printf("x-obs-meta-%s: %s\n", properties->meta_data[i].name, properties->meta_data[i].value);
        }
        */
        return OBS_STATUS_OK;
    }
    static void response_complete_callback(obs_status status, const obs_error_details *error, void *callback_data) {
        if (callback_data) {
            put_buffer_object_callback_data *data = (put_buffer_object_callback_data *)callback_data;
            data->ret_status = status;
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
                printf("Error Extra Detail(%d):\n   %s:%s\n", i, error->extra_details[i].name, error->extra_details[i].value);
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
    static int put_buffer_data_callback(int buffer_size, char *buffer, void *callback_data) {
        put_buffer_object_callback_data *data =
            (put_buffer_object_callback_data *)callback_data;
        int toRead = 0;
        if (data->buffer_size) {
            toRead = ((data->buffer_size > (unsigned)buffer_size) ? (unsigned)buffer_size : data->buffer_size);
            memcpy(buffer, data->put_buffer + data->cur_offset, toRead);
        }
        uint64_t originalContentLength = data->buffer_size;
        data->buffer_size -= toRead;
        data->cur_offset += toRead;
        if (data->buffer_size) {
            // printf("%llu bytes remaining ", (unsigned long long)data->buffer_size);
            double percentage = ((originalContentLength - data->buffer_size) * 100) / originalContentLength;
            // printf("(%d%% complete) ...\n", (int)(percentage * 100));
            print_progress(percentage / 100);
        } else {
            // printf("\n");
        }
        return toRead;
    }
};
