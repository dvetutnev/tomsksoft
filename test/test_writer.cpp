#include "writer.h"
#include "mocks.h"

#include <algorithm>


using ::testing::SaveArg;
using ::testing::NiceMock;
using ::testing::NotNull;
using ::testing::InSequence;
using ::testing::_;


TEST(Writer, write) {
    auto file = std::make_shared<NiceMock<MockFile>>();

    const std::string data = "asdfghjkl";
    std::unique_ptr<char[]> writenData = nullptr;
    EXPECT_CALL(*file, write(NotNull(), data.size(), 0)).WillOnce( [&writenData] (std::unique_ptr<char[]> p, auto, auto) { writenData = std::move(p); } );

    Writer<MockFile> writer{file};
    writer.push(data);

    ASSERT_TRUE(writenData);
    ASSERT_TRUE(std::equal(std::begin(data), std::end(data), writenData.get()));
}

TEST(Writer, defer) {
    auto file = std::make_shared<NiceMock<MockFile>>();

    MockHandle::THandler<uvw::FsEvent<uvw::FileReq::Type::WRITE>> handlerWriteEvent;
    EXPECT_CALL(*file, saveWriteHandler).WillOnce(SaveArg<0>(&handlerWriteEvent));

    const std::string prevData = "aaaaaawergv";
    const std::string data = "111asdfghjkl";
    std::unique_ptr<char[]> writenData = nullptr;

    EXPECT_CALL(*file, write).Times(1);
    EXPECT_CALL(*file, write(NotNull(), data.size(), prevData.size())).WillOnce( [&writenData] (std::unique_ptr<char[]> p, auto, auto) { writenData = std::move(p); } );

    Writer<MockFile> writer{file};

    writer.push(prevData);
    writer.push(data);

    uvw::FsEvent<uvw::FileReq::Type::WRITE> event{"file.txt", prevData.size()};
    handlerWriteEvent(event, *file);

    ASSERT_TRUE(writenData);
    ASSERT_TRUE(std::equal(std::begin(data), std::end(data), writenData.get()));
}

TEST(Writer, shutdown) {
    auto file = std::make_shared<NiceMock<MockFile>>();

    MockHandle::THandler<uvw::FsEvent<uvw::FileReq::Type::WRITE>> handlerWriteEvent;
    EXPECT_CALL(*file, saveWriteHandler).WillOnce(SaveArg<0>(&handlerWriteEvent));
    {
        InSequence _;
        EXPECT_CALL(*file, write).Times(1);
        EXPECT_CALL(*file, close).Times(1);
    }

    Writer<MockFile> writer{file};

    writer.push("aaaa");
    writer.shutdown();
    writer.push("bbbb");

    uvw::FsEvent<uvw::FileReq::Type::WRITE> event{"file.txt", 4};
    handlerWriteEvent(event, *file);
}

TEST(Writer, error) {
    auto file = std::make_shared<NiceMock<MockFile>>();

    MockHandle::THandler<uvw::ErrorEvent> handlerErrorEvent;
    EXPECT_CALL(*file, saveErrorHandler).WillOnce(SaveArg<0>(&handlerErrorEvent));

    Writer<MockFile> writer{file};

    uvw::ErrorEvent e{static_cast<std::underlying_type_t<uv_errno_t>>(UV_EFAULT)};
    ASSERT_DEATH({ handlerErrorEvent(e, *file); }, "");
}
