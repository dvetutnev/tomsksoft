#include "writer.h"
#include "mocks.h"

#include <algorithm>


using ::testing::SaveArg;
using ::testing::NotNull;
using ::testing::InSequence;
using ::testing::_;


TEST(Writer, write) {
    auto file = std::make_shared<MockFile>();

    const std::string data = "asdfghjkl";
    std::unique_ptr<char[]> writenData = nullptr;
    EXPECT_CALL(*file, write(NotNull(), data.size(), 0)).WillOnce( [&writenData] (std::unique_ptr<char[]> p, auto, auto) { writenData = std::move(p); } );

    Writer<MockFile> writer{file};
    writer.push(data);

    ASSERT_TRUE(writenData);
    ASSERT_TRUE(std::equal(std::begin(data), std::end(data), writenData.get()));
}

TEST(Writer, defer) {
    auto file = std::make_shared<MockFile>();

    MockHandle::THandler<uvw::FsEvent<uvw::FileReq::Type::WRITE>> handlerWriteEvent;
    EXPECT_CALL(*file, saveWriteHandler).WillOnce(SaveArg<0>(&handlerWriteEvent));

    const std::string prevData = "aaaaaawergv";
    const std::string data = "111asdfghjkl";
    std::unique_ptr<char[]> writenData = nullptr;
    {
        InSequence _;
        EXPECT_CALL(*file, write).Times(1);
        EXPECT_CALL(*file, write(NotNull(), data.size(), prevData.size())).WillOnce( [&writenData] (std::unique_ptr<char[]> p, auto, auto) { writenData = std::move(p); } );
    }

    Writer<MockFile> writer{file};
    writer.push(prevData);
    writer.push(data);

    uvw::FsEvent<uvw::FileReq::Type::WRITE> event{"file.txt", prevData.size()};
    handlerWriteEvent(event, *file);

    ASSERT_TRUE(writenData);
    ASSERT_TRUE(std::equal(std::begin(data), std::end(data), writenData.get()));
}
