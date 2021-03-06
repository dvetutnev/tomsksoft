* Консольное приложение сервер, имеющее настроечный файл (IP, port), принимающий произвольное кол-во клиентов.
* От клиента принимать произвольную строчку, выводить на экран и записывать её в файл.
* Иметь сборочный скрипт CMakeList.txt.
* Возможность запустить сервер в докер контейнере(должен прилагаться Docker.file).
* Исходники выложить на Git(lab/hub).
* Все запускается под Linux.
* Исходные коды тестового задания можно будет выложить в открытый доступ, и можете использовать их по своему усмотрению.

---

* Общение через TCP
* Клиент может несколько строчек отправить.
* Формат пакета : |uint |строка| (первые 4 байта это длина строки, за ними идет сама строка)
* Если клиент в течении 20 секунд ничего не присылает, считать соединение мертвым и закрывать его.

---

Делать будем с использованием *state-машин*. Такую мелкую штуку можно и без них написать, но тогда переходы между состояниями и условия переходов будут тонким слоем размазаны по всему проекту. С машинами состояний переходы (transitions), условия (guards) и действия (actions) будут собраны в декларативные таблички. Заодно этот проект пригодится в качестве примера для статьи "Как я перестал боятся и полюбил конечные автоматы".

# Сессия

Машина состояний:

![Session FSM](doc/session_fsm.jpg)

Структура сессии:

![Session classes](doc/session_classes.jpg)

А теперь небольшое пояснение зачем такая странная конструкция сделана. Связка [uvw](https://github.com/skypjack/uvw), к которой обработчики отдельно прицепляются на *handle*-лы и state-машины позволила получить полностью декларативный код, все состояния и переходы между ними описаны простой табличкой. В нем *ровно одно* ветвление: цикл для передачи в state-машину отдельных байтов. Если бы **uvw** позволяла по одному байту получать из сокета, то вообще без ветвлений получилось.

Реализация сессии:

```cpp
template <typename Server, typename Writer, typename Socket, typename Timer>
class Session : public SessionBase
{
public:
    Session(Server&, Writer&, std::shared_ptr<Socket>, std::shared_ptr<Timer>);

private:
    Server& server;
    Writer& writer;

    std::shared_ptr<Socket> client;
    std::shared_ptr<Timer> timer;


    std::string headerBuffer;
    std::uint32_t dataLength;
    std::string dataBuffer;

    void init();
    void pushToHeaderBuffer(char);
    bool isHeaderComplete() const;
    void processHeader();
    void pushToDataBuffer(char);
    bool isDataComplete() const;
    void processResult();
    void restart();
    void halt();

    struct DefFSM
    {
        explicit DefFSM(Session& s) : session{s} {}

        struct InitEvent {};
        struct ByteEvent { char byte; };
        struct HaltEvent {};

        auto operator()() const {

            auto init = [this] () { session.init(); };
            auto pushToHeaderBuffer = [this] (const ByteEvent& e) { session.pushToHeaderBuffer(e.byte); };
            auto isHeaderComplete = [this] () -> bool { return session.isHeaderComplete(); };
            auto processHeader = [this] () { session.processHeader(); };
            auto pushToDataBuffer = [this] (const ByteEvent& e) { session.pushToDataBuffer(e.byte); };
            auto isDataComplete = [this] () -> bool { return session.isDataComplete(); };
            auto processResult = [this] { session.processResult(); };
            auto restart = [this] { session.restart(); };
            auto halt = [this] { session.halt(); };

            using namespace boost::sml;

            return make_transition_table(
                *"Wait init"_s          + event<InitEvent> / init                     = "Receive header"_s,
                 "Receive header"_s     + event<ByteEvent> / pushToHeaderBuffer       = "Is header complete"_s,
                 "Is header complete"_s   [ !isHeaderComplete ]                       = "Receive header"_s,
                 "Is header complete"_s   [ isHeaderComplete  ]                       = "Process header"_s,
                 "Process header"_s                        / processHeader            = "Receive data"_s,
                 "Receive data"_s       + event<ByteEvent> / pushToDataBuffer         = "Is data complete"_s,
                 "Is data complete"_s     [ !isDataComplete ]                         = "Receive data"_s,
                 "Is data complete"_s     [ isDataComplete  ]                         = "Process result"_s,
                 "Process result"_s                        / (processResult, restart) = "Receive header"_s,

                *"In work"_s            + event<HaltEvent> / halt                     = X
            );
        }

        Session& session;
    };

    DefFSM defFsm;
    boost::sml::sm<DefFSM> fsm;
};


template <typename Server, typename Writer, typename Socket, typename Timer>
Session<Server, Writer, Socket, Timer>::Session(Server& s, Writer& w, std::shared_ptr<Socket> c, std::shared_ptr<Timer> t)
    :
      server{s},
      writer{w},

      client{std::move(c)},
      timer{std::move(t)},

      defFsm{*this},
      fsm{defFsm}
{
    auto dataHandler = [this] (const uvw::DataEvent& e, auto&) {
        for (std::size_t i = 0; i < e.length; i++) {
            typename DefFSM::ByteEvent byteEvent{e.data[i]};
            fsm.process_event(byteEvent);
        }
    };
    client->template on<uvw::DataEvent>(dataHandler);

    auto errorHandler = [this] (const uvw::ErrorEvent&, auto&) { fsm.process_event(typename DefFSM::HaltEvent{}); };
    client->template on<uvw::ErrorEvent>(errorHandler);

    auto timeoutHandler = [this] (const uvw::TimerEvent&, auto&) { fsm.process_event(typename DefFSM::HaltEvent{}); };
    timer->template on<uvw::TimerEvent>(timeoutHandler);

    fsm.process_event(typename DefFSM::InitEvent{});
}
```

Методы Session, которые дергает state-машина не показаны, почти все они однострочные и логики там нет, вся логика собрана *transition table*. Класс Session шаблонизирован для подстановки моков во время тестирования (статический полиморфизм).

# Обработка данных

Машина состояний довольно простая (обработка ошибок не показанна):

![Writer FSM](doc/writer_fsm.jpg)

И тут опять очень пригодились возможности state-машины: если во время записи прилетит еще один эвент, то он поместиться в очередь. По завершению записи данных предыдущего эвента отложенный эвент извлекается из очереди (если их несколько прилетело, то сохраняется порядок) и обрабатывается. И все это происходит автоматически)) Открытие файла будет для упрощения синхронным (при инстанционировании класса Writer), обработка ошибок ФС - **std::abort** (пусть с этим разбирается то, что запустило этот сервер, на уровне возникновения проблема не решается). Разумеется можно прикрутить открытие файла по приходу первого эвента с данными, но для экономии времени на текущий момент реализовавать такую штуку не буду. Реализация:

```cpp
template <typename File>
class Writer
{
public:
    Writer(std::shared_ptr<File>);

    void push(const std::string&);
    void shutdown();

private:
    std::shared_ptr<File> file;

    void writeToFile(const std::string&);
    void closeFile();

    std::int64_t offset = 0;

    struct DefFSM
    {
        explicit DefFSM(Writer& w) : writer{w} {}

        struct DataEvent { std::string data; };
        struct WrittenEvent {};
        struct ShutdownEvent {};

        auto operator()() const {

            auto write = [this] (const DataEvent& e) { writer.writeToFile(e.data); };
            auto close = [this] (const ShutdownEvent&) { writer.closeFile(); };

            using namespace boost::sml;

            return make_transition_table(
                *"Wait"_s   + event<DataEvent>      / write = "Write"_s,
                 "Wait"_s   + event<ShutdownEvent>  / close = X,
                 "Write"_s  + event<DataEvent>      / defer,
                 "Write"_s  + event<ShutdownEvent>  / defer,
                 "Write"_s  + event<WrittenEvent>           = "Wait"_s
            );
        }

        Writer& writer;
    };

    DefFSM defFsm;
    boost::sml::sm<DefFSM, boost::sml::defer_queue<std::deque>> fsm;
};
```

Благодаря state-машине довольно легко реализовалось *чистое* завершение, после вызова метода **shutdown** Writer перестает принимать входящие сообщения, дописывает в файл то, что ему накидали (лежит в очереди state-машины) и закрывает файл. Подтверждающий тест:

```cpp
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
```

# Сервер

При запуске инстанционирует ожидающией сокет и **Writer**. При подключение клиента инстанционирует клиентский сокет и **Session** передавая ей клиентский сокет. Сессия при отлючении клиента удаляет себя из сервера (метод **remove**). При остановке сервер закрывает ожидающий сокет и пришибает сессии (метод **halt**). Реализация (тут я уж не стал тестами обкладывать):

```cpp
class Server
{
public:
    Server(uvw::Loop& loop, std::string ip, unsigned int port, const std::filesystem::path&);

    void remove(SessionBase*);
    void stop();

private:
    uvw::Loop& loop;

    std::shared_ptr<uvw::TCPHandle> listener;

    using W = Writer<uvw::FileReq>;
    std::shared_ptr<W> writer;

    using S = Session<Server, W, uvw::TCPHandle, uvw::TimerHandle>;
    std::set<std::shared_ptr<SessionBase>> connections;
};


inline Server::Server(uvw::Loop& loop, std::string ip, unsigned int port, const std::filesystem::path& path)
    :
      loop{loop}
{
    listener = loop.resource<uvw::TCPHandle>();

    auto onConnect = [this] (const uvw::ListenEvent&, auto&) {
        auto client = this->loop.resource<uvw::TCPHandle>();
        this->listener->accept(*client);

        auto timer = this->loop.resource<uvw::TimerHandle>();

        auto session = std::make_shared<S>(*this, *writer, std::move(client), std::move(timer));
        this->connections.insert(std::move(session));
    };
    listener->on<uvw::ListenEvent>(onConnect);

    auto onError = [] (const uvw::ErrorEvent& e, auto&) {
        std::cout << "Server error: " << e.what() << std::endl;
        std::abort();
    };
    listener->on<uvw::ErrorEvent>(onError);

    listener->bind("127.0.0.1", 4242);
    listener->listen();

    auto file = loop.resource<uvw::FileReq>();
    file->openSync(path, O_CREAT | O_RDWR, 0644);
    writer = std::make_shared<W>(std::move(file));
}

inline void Server::remove(SessionBase* s) {
    auto pred = [s] (const std::shared_ptr<SessionBase>& conn) -> bool {
        return conn.get() == s;
    };
    auto it = std::find_if(std::begin(connections), std::end(connections), pred);
    assert(it != std::end(connections));
    connections.erase(it);
}

inline void Server::stop() {
    listener->close();

    auto copy = connections;
    for (auto& session : copy) {
        session->halt();
    }

    std::cout << "Server in stop" << std::endl;
}
```

Функциональный тест:

```cpp
TEST(Functional, _) {
    const std::filesystem::path path = "log.txt";
    if (std::filesystem::exists(path)) {
        std::filesystem::remove(path);
    }

    auto loop = uvw::Loop::getDefault();

    Server server{*loop, "127.0.0.1", 4242, path};

    auto socket = loop->resource<uvw::TCPHandle>();
    auto onConnect = [](const uvw::ConnectEvent&, uvw::TCPHandle& socket){
        uvw::DataEvent packet = createPacket("sTRINg");
        socket.write(std::move(std::move(packet.data)), packet.length);
        socket.close();
    };
    auto onError = [](const uvw::ErrorEvent& e, auto&) {
        FAIL() << "Connect failed: " << e.what();
    };

    socket->once<uvw::ConnectEvent>(onConnect);
    socket->on<uvw::ErrorEvent>(onError);
    socket->connect(std::string{"127.0.0.1"}, 4242);

    auto timer = loop->resource<uvw::TimerHandle>();
    auto onTimer = [&server, &timer](const uvw::TimerEvent&, auto&) {
        server.stop();
        timer->close();
    };
    timer->on<uvw::TimerEvent>(onTimer);

    timer->start(std::chrono::milliseconds{100}, std::chrono::milliseconds{0});


    loop->run();


    std::fstream file{path};
    std::string log;
    file >> log;

    ASSERT_EQ(log, "sTRINg");
}
```

# Промежуточные выводы

При выполнении этой ТЗшки я решил поэкспериментировать (поумничать) и опробовать новую для себя библиотеку *state-машин* [Boost.SML](https://boost-ext.github.io/sml/index.html) и статический полиморфизм для подмены объектов асинхронного IO.

[Boost.SML](https://boost-ext.github.io/sml/index.html) удивительная вещь. Очень редко когда с новой библиотекой все получается *сразу*, вот с ней очень даже. А описание таблицы состояний прямо в нотации **UML** заслуживает отдельного лайка. Вот что дает перегрузка операторов, можно без всяких парсеров реализовать нужный синтаксис.

Статитеский полиморфизм это удобно. Классы **Session** и **Writer** писались и отлаживались на mock-объектах (таймер\сокет\файл), с реальными классами из **uvw** оно заработало сразу. Хотя казалось бы, mock-ки и рабочие классы даже общей базы не имеют. Но документацию на мокируемые объекты нужно читать внимательно и не стесняться для mock-а написать тест.

# Контейнер

Сборка и запуск контейнера:

```
docker build -t tomsksoft .
docker run -i -t --rm -p 6789:6789 tomsksoft

Address: 0.0.0.0, port: 6789, log: "log.txt"
dddd
aaaa
^CServer in stop

cat log.txt
ddddaaaa
```

Отправка сообщений в сервер:

```
build/tomsksoft-client 127.0.0.1 6789 dddd aaaa
```
