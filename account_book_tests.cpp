#include <gtest/gtest.h>

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <map>
#include <regex>
#include <string>
#include <vector>

// ===================== 被测子功能：工具函数（最小实现） =====================
static std::string GetCurrentDate() {
    std::time_t now = std::time(nullptr);

    std::tm ltm{};
#if defined(_WIN32)
    localtime_s(&ltm, &now);
#else
    localtime_r(&now, &ltm);
#endif

    char buf[20];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
                  1900 + ltm.tm_year, 1 + ltm.tm_mon, ltm.tm_mday);
    return std::string(buf);
}

// ===================== 被测子功能：分类识别（最小实现） =====================
struct Category {
    int id;
    std::string name;
    std::string description;
};

class CategoryRecognizer {
 public:
    explicit CategoryRecognizer(const std::vector<Category>& categories)
        : categories_(categories) {}

    int RecognizeCategory(const std::string& note) {
        // 简单关键词匹配：命中分类名即返回对应 id
        std::map<std::string, int> keyword_map;
        for (const auto& c : categories_) keyword_map[c.name] = c.id;

        for (const auto& kv : keyword_map) {
            if (note.find(kv.first) != std::string::npos) return kv.second;
        }

        // 默认“其他”
        for (const auto& c : categories_) {
            if (c.name == "其他") return c.id;
        }

        // 再退化：无“其他”则返回第一个；无分类返回 0
        return categories_.empty() ? 0 : categories_[0].id;
    }

 private:
    std::vector<Category> categories_;
};

// ===================== 单元测试：GetCurrentDate() =====================
TEST(GetCurrentDateTests, FormatIsYYYYMMDD) {
    std::string d = GetCurrentDate();
    EXPECT_TRUE(std::regex_match(d, std::regex(R"(^\d{4}-\d{2}-\d{2}$)")));
}

TEST(GetCurrentDateTests, LengthIs10) {
    std::string d = GetCurrentDate();
    EXPECT_EQ(d.size(), 10u);
}

TEST(GetCurrentDateTests, HasDashesAtExpectedPositions) {
    std::string d = GetCurrentDate();
    ASSERT_GE(d.size(), 10u);
    EXPECT_EQ(d[4], '-');
    EXPECT_EQ(d[7], '-');
}

TEST(GetCurrentDateTests, MonthBetween01And12) {
    std::string d = GetCurrentDate();
    int month = std::stoi(d.substr(5, 2));
    EXPECT_GE(month, 1);
    EXPECT_LE(month, 12);
}

TEST(GetCurrentDateTests, DayBetween01And31) {
    std::string d = GetCurrentDate();
    int day = std::stoi(d.substr(8, 2));
    EXPECT_GE(day, 1);
    EXPECT_LE(day, 31);
}

TEST(GetCurrentDateTests, MultipleCallsStillValid) {
    for (int i = 0; i < 10; ++i) {
        std::string d = GetCurrentDate();
        EXPECT_TRUE(std::regex_match(d, std::regex(R"(^\d{4}-\d{2}-\d{2}$)")));
    }
}

// ===================== 单元测试：CategoryRecognizer::RecognizeCategory() =====================
static std::vector<Category> DefaultCats() {
    return {
        {1, "餐饮", "饮食相关"},
        {2, "娱乐", "娱乐消费"},
        {3, "水电费", "生活缴费"},
        {4, "工资", "收入"},
        {5, "其他", "其他"},
    };
}

TEST(CategoryRecognizerTests, MatchExactKeywordAtBeginning) {
    CategoryRecognizer cr(DefaultCats());
    EXPECT_EQ(cr.RecognizeCategory("餐饮 午饭"), 1);
}

TEST(CategoryRecognizerTests, MatchKeywordInMiddle) {
    CategoryRecognizer cr(DefaultCats());
    EXPECT_EQ(cr.RecognizeCategory("今天去餐饮店吃饭"), 1);
}

TEST(CategoryRecognizerTests, MatchAnotherCategory) {
    CategoryRecognizer cr(DefaultCats());
    EXPECT_EQ(cr.RecognizeCategory("娱乐 电影票"), 2);
}

TEST(CategoryRecognizerTests, MatchUtilitiesCategory) {
    CategoryRecognizer cr(DefaultCats());
    EXPECT_EQ(cr.RecognizeCategory("水电费 1月账单"), 3);
}

TEST(CategoryRecognizerTests, MatchIncomeCategory) {
    CategoryRecognizer cr(DefaultCats());
    EXPECT_EQ(cr.RecognizeCategory("工资 发放"), 4);
}

TEST(CategoryRecognizerTests, NoKeywordFallsBackToOther) {
    CategoryRecognizer cr(DefaultCats());
    EXPECT_EQ(cr.RecognizeCategory("买书"), 5);
}

TEST(CategoryRecognizerTests, EmptyNoteFallsBackToOther) {
    CategoryRecognizer cr(DefaultCats());
    EXPECT_EQ(cr.RecognizeCategory(""), 5);
}

TEST(CategoryRecognizerTests, CategoriesWithoutOtherFallsBackToFirst) {
    std::vector<Category> cats = {
        {10, "餐饮", ""},
        {11, "娱乐", ""},
    };
    CategoryRecognizer cr(cats);
    EXPECT_EQ(cr.RecognizeCategory("完全不匹配"), 10);
}

TEST(CategoryRecognizerTests, EmptyCategoriesReturns0) {
    std::vector<Category> cats;
    CategoryRecognizer cr(cats);
    EXPECT_EQ(cr.RecognizeCategory("任意"), 0);
}

TEST(CategoryRecognizerTests, MultipleKeywordsReturnsOneOfExpected) {
    CategoryRecognizer cr(DefaultCats());
    int cid = cr.RecognizeCategory("餐饮+娱乐");
    EXPECT_TRUE(cid == 1 || cid == 2);
}

TEST(CategoryRecognizerTests, NoteContainsOtherKeywordShouldReturnOther) {
    CategoryRecognizer cr(DefaultCats());
    EXPECT_EQ(cr.RecognizeCategory("其他: 杂项支出"), 5);
}

// ===================== GTest 入口（保证此文件可单独编译运行） =====================
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
