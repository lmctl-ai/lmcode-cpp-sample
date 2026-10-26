// Native boundaries are deliberately explicit: bytes are not Unicode strings,
// floats stay binary64 (including signed zero/non-finite values), and object
// identity is distinct from equal contents. Std collection arenas own returns.
#include "shop.hpp"
#include "collections.hpp"
#include <limits>

namespace sample {
void register_conversions(dslvm::Registry& r) {
    using dslvm::Args;
    using dslvm::Value;
    r.function("host_int", {"int"}, "int", [](Args& a) { return Value::integer(a[0].i); });
    r.function("host_float", {"float"}, "float", [](Args& a) { return Value::real(a[0].f); });
    r.function("host_bool", {"bool"}, "bool", [](Args& a) { return Value::boolean(a[0].b); });
    r.function("host_bytes", {"string"}, "string", [](Args& a) { return Value::string(a[0].s); });
    r.function("host_hex", {"string"}, "string", [](Args& a) { return Value::string(dslvm::hex(a[0].s)); });
    r.function("host_float_case", {"string"}, "float", [](Args& a) {
        const auto& name = a[0].s;
        if (name == "-0") return Value::real(-0.0);
        if (name == "nan") return Value::real(std::numeric_limits<double>::quiet_NaN());
        if (name == "inf") return Value::real(std::numeric_limits<double>::infinity());
        if (name == "-inf") return Value::real(-std::numeric_limits<double>::infinity());
        if (name == "large") return Value::real(1e21);
        if (name == "subnormal") return Value::real(std::numeric_limits<double>::denorm_min());
        throw dslvm::Error("host_float_case: unknown case");
    });
    r.function("host_echo_money", {"Money"}, "Money", [](Args& a) { return a[0]; });
    r.function("host_same", {"Money", "Money"}, "bool", [](Args& a) { return Value::boolean(a[0].obj == a[1].obj); });
    r.function("host_null", {}, "Money", [](Args&) { return Value::object("Money", nullptr); });
    r.function("host_is_null", {"Money"}, "bool", [](Args& a) { return Value::boolean(a[0].obj == nullptr); });
    r.function("host_wrong", {}, "Money", [](Args&) { return Value::object("Items", nullptr); });
    r.function("host_void", {}, "void", [](Args&) { return Value::none(); });
    r.function("host_str_list", {"StrList"}, "StrList", [&r](Args& a) {
        auto values = static_cast<dslvm::stdlib::StrList*>(a[0].obj)->items;
        values.push_back("host");
        return Value::object("StrList", r.std_collections().make_str_list(std::move(values)));
    });
    r.function("host_int_list", {"IntList"}, "IntList", [&r](Args& a) {
        auto values = static_cast<dslvm::stdlib::IntList*>(a[0].obj)->items;
        values.push_back(9);
        return Value::object("IntList", r.std_collections().make_int_list(std::move(values)));
    });
    r.function("host_float_list", {"FloatList"}, "FloatList", [&r](Args& a) {
        auto values = static_cast<dslvm::stdlib::FloatList*>(a[0].obj)->items;
        values.push_back(0.5);
        return Value::object("FloatList", r.std_collections().make_float_list(std::move(values)));
    });
    r.function("host_str_map", {"StrMap"}, "StrMap", [&r](Args& a) {
        auto* output = r.std_collections().make_str_map();
        for (const auto& item : static_cast<dslvm::stdlib::StrMap*>(a[0].obj)->entries())
            output->set(item.first, item.second);
        output->set("host", "yes");
        return Value::object("StrMap", output);
    });
    r.function("host_sum", std::vector<std::string>(255, "int"), "int", [](Args& a) {
        uint64_t sum = 0;
        for (const auto& value : a) sum += uint64_t(value.i);
        return Value::integer(dslvm::from_twos_complement(sum));
    });
    r.function("host_long_" + std::string(128, 'x'), {"int"}, "int", [](Args& a) { return Value::integer(a[0].i); });
}
} // namespace sample
