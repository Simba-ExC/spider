#include "HtmlClient.h"

static Message checkResult(http::response<http::dynamic_body> res)
{
    std::wstring answer;
    unsigned int errCode = res.base().result_int();
    switch (errCode)
    {
        case 301:
        {
            std::string url(res.base()["Location"]);
            //do_request(url);
            answer = utf82wideUtf(url);
            break;
        }
        case 200:
        {
            std::stringstream ss;
            ss << res;
            std::string s(ss.str());
            answer = utf82wideUtf(s);
            break;
        }
        default:
            //throw std::runtime_error("Unexpected HTTP status " + std::to_string(errCode));
            answer = L"Unexpected HTTP status " + std::to_wstring(errCode);
            break;
    }

    return { errCode, answer };
}

static Message httpsRequest(const tcp::resolver::results_type& sequenceEp,
    const http::request<http::string_body>& req, net::io_context& ioc)
{
    ssl::context ctx(ssl::context::sslv23);
    ctx.set_default_verify_paths();
    ctx.set_options(ssl::context::default_workarounds | ssl::context::verify_none);

    ssl::stream<tcp::socket> sslStream(ioc, ctx);
    sslStream.set_verify_mode(ssl::context::verify_none);
    sslStream.set_verify_callback([](bool, ssl::verify_context&) {return true; });
    boost::asio::connect(sslStream.lowest_layer(), sequenceEp);
    sslStream.handshake(ssl::stream<tcp::socket>::client);
    sslStream.lowest_layer().set_option(tcp::no_delay(true));
    int bytes_sent = http::write(sslStream, req);
    beast::flat_buffer buffer;
    http::response<http::dynamic_body> res;
    auto bytes_received = http::read(sslStream, buffer, res);
    // Аккуратно закройте сокет
    beast::error_code ec;
    sslStream.shutdown(ec);
    if (ec == ssl::error::stream_truncated)
        ec = {};
    sslStream.lowest_layer().shutdown(tcp::socket::shutdown_both, ec);
    sslStream.lowest_layer().close();
    if (ec && ec != beast::errc::not_connected)
        throw beast::system_error{ ec };

    return checkResult(res);
}

static Message httpRequest(const tcp::resolver::results_type& sequenceEp,
    const http::request<http::string_body>& req, net::io_context& ioc)
{
    beast::tcp_stream stream{ ioc };
    boost::asio::connect(stream.socket(), sequenceEp);
    int bytes_sent = http::write(stream, req);
    beast::flat_buffer buffer;
    // Объявите контейнер для хранения ответа
    http::response<http::dynamic_body> res;
    int bytes_received = http::read(stream, buffer, res);
    // Аккуратно закройте сокет
    beast::error_code ec;
    stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    stream.socket().close();
    if (ec && ec != beast::errc::not_connected)
        throw beast::system_error{ ec };

    return checkResult(res);
}

static Message do_request(const std::string urlStr)
{
    Message mes{ 0, L""};
    try
    {
        // Парсинг строки ссылки
        boost::url url = boost::urls::parse_uri(urlStr).value();
        if (url.path().empty())
            url.set_path("/");

        // io_context требуется для всех операций ввода-вывода.
        net::io_context ioc;
        // Список конечных точек
        tcp::resolver resolver{ ioc };
        tcp::resolver::query query(url.host(), url.scheme());
        tcp::resolver::results_type sequenceEp = resolver.resolve(query);
        int port = sequenceEp->endpoint().port();

        // Настройка запроса HTTP GET
        http::request<http::string_body> request;
        request.method(http::verb::get);
        request.target(url.path());
        request.version(11);
        request.keep_alive(true);
        request.set(http::field::host, url.host());
        request.set(http::field::user_agent, "Spider");
        mes = (port == 443) ? httpsRequest(sequenceEp, request, ioc)
            : httpRequest(sequenceEp, request, ioc);
    }
    catch (const boost::detail::with_throw_location<boost::system::system_error>& err)
    { /* Ошибка в синтаксисе URL -> игнор. */
        std::wstring answer = L"Исключение типа: " + utf82wideUtf(typeid(err).name())
            + L"\nОшибка: " + utf82wideUtf(err.what());
        mes = { 1, answer };
    }
    catch (const boost::wrapexcept<boost::system::system_error>& err)
    { /* Удаленный сервер, отверг соединение -> игнор. */
        std::wstring answer = L"Исключение типа: " + utf82wideUtf(typeid(err).name())
            + L"\nОшибка: " + ansi2wideUtf(err.what());
        mes = { 1, answer };
    }
    catch (const std::exception& err)
    {
        std::wstring answer = L"Исключение типа: " + utf82wideUtf(typeid(err).name())
            + L"\nОшибка: " + utf82wideUtf(err.what());
        mes = { 1, answer };
    }

    return mes;
}
Message HtmlClient::getRequest(const std::string& urlStr)
{
    return do_request(urlStr);
}
