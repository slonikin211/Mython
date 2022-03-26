#include "runtime.h"

#include <cassert>
#include <sstream>

using namespace std;

namespace runtime {

ObjectHolder::ObjectHolder(std::shared_ptr<Object> data)
    : data_(std::move(data)) {
}

void ObjectHolder::AssertIsValid() const {
    assert(data_ != nullptr);
}

ObjectHolder ObjectHolder::Share(Object& object) {
    // Возвращаем невладеющий shared_ptr (его deleter ничего не делает)
    return ObjectHolder(std::shared_ptr<Object>(&object, [](auto* /*p*/) { /* do nothing */ }));
}

ObjectHolder ObjectHolder::None() {
    return ObjectHolder();
}

Object& ObjectHolder::operator*() const {
    AssertIsValid();
    return *Get();
}

Object* ObjectHolder::operator->() const {
    AssertIsValid();
    return Get();
}

Object* ObjectHolder::Get() const {
    return data_.get();
}

ObjectHolder::operator bool() const {
    return Get() != nullptr;
}

bool IsTrue(const ObjectHolder& object) {
    if (object.TryAs<ValueObject<bool>>())
    {
        return object.TryAs<ValueObject<bool>>()->GetValue();
    }
    if (object.TryAs<ValueObject<int>>())
    {
        return object.TryAs<ValueObject<int>>()->GetValue() != 0;
    }
    if (object.TryAs<ValueObject<string>>())
    {
        return object.TryAs<ValueObject<string>>()->GetValue() != ""s;
    }
    return false;
}



namespace special_methods
{
    const string kStr = "__str__"s;
    const string kEqual = "__eq__"s;
    const string kLess = "__lt__"s; 
}

void ClassInstance::Print(std::ostream& os, Context& context) {
    if (this->HasMethod(special_methods::kStr, 0u))
    {
        this->Call(special_methods::kStr, {}, context)->Print(os, context);
    }
    else
    {
        os << this;
    }
}

bool ClassInstance::HasMethod(const std::string& method, size_t argument_count) const {
    if (const Method* class_method = linked_class_.GetMethod(method))
    {
        if (argument_count == class_method->formal_params.size())
        {
            return true;
        }
    }
    return false;
}

Closure& ClassInstance::Fields() {
    return closure_;
}

const Closure& ClassInstance::Fields() const {
    return closure_;
}

ClassInstance::ClassInstance(const Class& cls) 
    :linked_class_(cls) {}

ObjectHolder ClassInstance::Call(const std::string& method,
                                 const std::vector<ObjectHolder>& actual_args,
                                 Context& context) 
{
    const Method* class_method = linked_class_.GetMethod(method);
    if (class_method == nullptr)
    {
        throw std::runtime_error("Not implemented"s);
    }
    else if (class_method->formal_params.size() != actual_args.size())
    {
        throw std::runtime_error("Not implemented"s);
    }

    Closure closure;
    closure["self"] = ObjectHolder::Share(*this);

    for (size_t i = 0u; i < class_method->formal_params.size(); ++i)
    {
        closure[class_method->formal_params.at(i)] = actual_args.at(i);
    }
    return class_method->body->Execute(closure, context);
}

Class::Class(std::string name, std::vector<Method> methods, const Class* parent)
    :name_(move(name)), methods_(move(methods)), parent_(parent) {}

const Method* Class::GetMethod(const std::string& name) const {
    for (const auto& method: methods_)
    {
        if (method.name == name)
        {
            return &method;
        }
    }
    if (parent_ != nullptr)
    {
        return parent_->GetMethod(name);
    }
    return nullptr;
}

void Class::Print(ostream& os, [[maybe_unused]] Context& context) {
    os << "Class " << name_;
}

void Bool::Print(std::ostream& os, [[maybe_unused]] Context& context) {
    os << (GetValue() ? "True"sv : "False"sv);
}

bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
   
    if (optional<bool> res = Comparer(lhs, rhs, equal_to<>{}); res.has_value())
    {
        return res.value();
    }


    if (!lhs && !rhs)
    {
        return true;
    }
    if (lhs.TryAs<ClassInstance>())
    {
        if (lhs.TryAs<ClassInstance>()->HasMethod(special_methods::kEqual, 1u))
        {
            return IsTrue(lhs.TryAs<ClassInstance>()->Call(special_methods::kEqual, { rhs }, context));
        }
    }
    throw std::runtime_error("Cannot compare objects for equality"s);
}

bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {

    if (optional<bool> res = Comparer(lhs, rhs, less<>{}))
    {
        return res.value();
    }

    if (lhs.TryAs<ClassInstance>()) 
    {
        if (lhs.TryAs<ClassInstance>()->HasMethod(special_methods::kLess, 1U)) 
        {
            return IsTrue(lhs.TryAs<ClassInstance>()->Call(special_methods::kLess, { rhs }, context));
        }
    }
    throw std::runtime_error("diffrent tipes"s);
}

bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Equal(lhs, rhs, context);
}

bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Less(lhs, rhs, context) && NotEqual(lhs, rhs, context);
}

bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return (Less(lhs, rhs, context) || Equal(lhs, rhs, context));
}

bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return (!Less(lhs, rhs, context));
}

}  // namespace runtime