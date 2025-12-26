#include "huawei_obs.h"
#include "log.h"
#include <gtest/gtest.h>
#include <string>
#include <random>

class HuaweiCloudObsTest : public ::testing::Test {
protected:
    HuaweiCloudObs * obs_client;

    // 生成随机 Key 以避免测试冲突
    std::string generate_random_key(const std::string& prefix) {
        static const char charset[] = "0123456789abcdefghijklmnopqrstuvwxyz";
        static std::mt19937 gen(std::random_device{}());
        static std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);

        std::string result = prefix + "_";
        result.resize(prefix.size() + 1 + 8);
        for (size_t i = prefix.size() + 1; i < result.size(); ++i) {
            result[i] = charset[dist(gen)];
        }
        return result;
    }

    // 生成随机数据
    std::string generate_data(size_t size) {
        std::string data(size, 'a');
        return data;
    }

    void SetUp() override {
        obs_client = HuaweiCloudObs::get_instance();
    }
};

// 测试 Put Object 功能
TEST_F(HuaweiCloudObsTest, PutObjectSuccess) {
    std::string key = generate_random_key("unittest_put");
    std::string data = "Hello OBS, this is a put test.";

    EXPECT_NO_THROW(obs_client->put_object(key, data));
    EXPECT_NO_THROW(obs_client->delete_objects({key}));
}

// 测试 Append Object 功能
TEST_F(HuaweiCloudObsTest, AppendObjectSuccess) {
    std::string key = generate_random_key("unittest_append");
    std::string part1 = "Hello ";
    std::string part2 = "World!";

    // 第一次追加，起始位置必须为 0
    size_t next_pos = 0;
    EXPECT_NO_THROW({
        next_pos = obs_client->append_object(key, part1, 0);
    });

    // 验证返回的下一次追加位置是否正确
    EXPECT_EQ(next_pos, part1.size());

    // 第二次追加，起始位置为上一次的返回值
    EXPECT_NO_THROW({
        next_pos = obs_client->append_object(key, part2, next_pos);
    });

    // 验证最终大小
    EXPECT_EQ(next_pos, part1.size() + part2.size());
    EXPECT_NO_THROW(obs_client->delete_object(key));
}

TEST_F(HuaweiCloudObsTest, PutLargeObject) {
    std::string key = generate_random_key("unittest_large");
    std::string data = generate_data(1024 * 1024); // 1MB

    EXPECT_NO_THROW(obs_client->put_object(key, data));
    EXPECT_NO_THROW(obs_client->delete_objects({key}));
}

TEST_F(HuaweiCloudObsTest, Throw) {
    std::string key = generate_random_key("unittest_throw");
    std::string data = "Hello OBS, this is a put test.";
    EXPECT_THROW(obs_client->append_object(key, data, 1000), std::exception);
}

// ./hw_obs_test --gtest_filter=HuaweiCloudObsTest.DeleteAll
TEST_F(HuaweiCloudObsTest, DeleteAll) {
    obs_client->delete_all();
}

int main(int argc, char **argv) {
    init_logger();
    init_all_config();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}