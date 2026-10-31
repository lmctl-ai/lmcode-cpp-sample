// Native host protocol. Compilation belongs to lmscript; execution belongs here.
#include "shop.hpp"
#include "sample_schema.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <locale>
#include <sstream>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

namespace {
std::string read_bytecode(const std::string& path) {
    std::ifstream input(std::filesystem::u8path(path), std::ios::binary);
    if (!input) throw std::runtime_error("cannot read bytecode");
    std::ostringstream bytes;
    bytes << input.rdbuf();
    if (input.bad()) throw std::runtime_error("cannot read bytecode");
    return bytes.str();
}
std::string value_text(const dslvm::Value& v) {  // same text as the language's str()
    if (v.kind == dslvm::Value::INT) return std::to_string(v.i);
    if (v.kind == dslvm::Value::FLOAT) { char b[64]; std::snprintf(b, sizeof b, "%.15g", v.f); return b; }
    if (v.kind == dslvm::Value::BOOL) return v.b ? "true" : "false";
    return v.s;
}
int usage() {
    std::cout << "{\"ok\": false, \"phase\": \"cli\", \"code\": \"CLI_USAGE\", \"message\": "
              << dslvm::json_escape("usage: --version | --dsl-schema [--json] | --dsl-signatures | --dsl FILE") << "}\n";
    return 2;
}
}

int main(int argc, char** argv) {
#ifdef _WIN32
    _setmode(_fileno(stdout), _O_BINARY); // The protocol is LF bytes, never CRLF.
#endif
    std::cout.imbue(std::locale::classic());
    try {
        sample::Host host;
        if (argc == 2 && std::string(argv[1]) == "--version") {
            std::cout << "lmcode-cpp-sample " << SAMPLE_VERSION << "\n";
            return 0;
        }
        if ((argc == 2 || (argc == 3 && std::string(argv[2]) == "--json")) &&
            std::string(argv[1]) == "--dsl-schema") {
            std::cout.write(sample::schema, sizeof(sample::schema) - 1);
            return 0;
        }
        if (argc == 2 && std::string(argv[1]) == "--dsl-signatures") {
            for (const auto& signature : host.registry.signatures()) std::cout << signature << '\n';
            return 0;
        }
        if (argc != 3 || std::string(argv[1]) != "--dsl") return usage();

        dslvm::Program program;
        try {
            program = dslvm::Program::load(read_bytecode(argv[2]), sample::schema, host.registry);
        } catch (const dslvm::Error& error) {
            // Loader wording is deliberately normalized so all eight ports agree.
            std::string message = error.code == "LOAD_SCHEMA_MISMATCH" ? "sample schema mismatch" :
                                  error.code == "LOAD_UNBOUND" ? "sample import mismatch" : "invalid sample bytecode";
            return dslvm::report_error("load", dslvm::Error(error.code, message));
        }
        auto context = host.context();
        dslvm::ResultSink invocation_result;
        host.registry.set_result_sink(&invocation_result); // body + all selected entries
        try {
            program.run(context, host.registry);
        } catch (const dslvm::Error& error) {
            return dslvm::report_error("run", error);
        }
        // Rule entries (DSLB v6): after the body, @item entries run once per shop
        // item in order, @total once with the shop total, other annotations never.
        size_t entries_called = 0;
        auto call_entry = [&](size_t index, dslvm::Value argument) {
            const dslvm::Program::Entry& entry = program.entries()[index];
            dslvm::Value result = program.call(index, context, host.registry, {std::move(argument)});
            ++entries_called;
            if (result.kind != dslvm::Value::VOID) {
                std::cout << "entry " << entry.name << " = " << value_text(result) << "\n";
            }
        };
        try {
            for (size_t i = 0; i < program.entries().size(); ++i) {
                const std::string& annotation = program.entries()[i].annotation;
                if (annotation == "item") {
                    for (auto* item : host.item_handles()) call_entry(i, dslvm::Value::object("Money", item));
                } else if (annotation == "total") {
                    call_entry(i, host.total_handle());
                }
            }
        } catch (const dslvm::Error& error) {
            return dslvm::report_error("run", error);
        }
        // Application validation is distinct from a compiler/loader/VM failure.
        if (host.ledger.balance < 0) return dslvm::report_app_failure("ledger: negative balance");
        const int status = dslvm::report_ok("{\"balance\": " + std::to_string(host.ledger.balance) +
                               ", \"entries\": " + std::to_string(host.ledger.entries) +
                               ", \"note\": " + dslvm::json_escape(host.ledger.note) +
                               ", \"entries_called\": " + std::to_string(entries_called) + "}");
        if (status) return status;
        dslvm::write_result_frame(std::cout, invocation_result);
        return 0;
    } catch (const std::exception&) {
        return dslvm::report_app_failure("cannot read sample input");
    }
}
