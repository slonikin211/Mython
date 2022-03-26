#include "statement.h"

#include <iostream>
#include <sstream>

using namespace std;

namespace ast {

using runtime::Closure;
using runtime::Context;
using runtime::ObjectHolder;

namespace {
const string ADD_METHOD = "__add__"s;
const string INIT_METHOD = "__init__"s;
}  // namespace

ObjectHolder Assignment::Execute(Closure& closure, Context& context) {
    return closure[var_] = rv_->Execute(closure, context);
}

Assignment::Assignment(std::string var, std::unique_ptr<Statement> rv)
    :var_(var), rv_(move(rv)) {}

VariableValue::VariableValue(const std::string& var_name) 
    :value_(var_name) {}

VariableValue::VariableValue(std::vector<std::string> dotted_ids) 
    :value_(move(dotted_ids)) {}

ObjectHolder VariableValue::Execute(Closure& closure, Context& context) {
    if (holds_alternative<const string>(value_))
    {
        const string value = std::get<const string>(value_);
        if (closure.find(value) != closure.end())
        {
            return closure.at(value);
        }
    }
    else if (holds_alternative<vector<string>>(value_))
    {
        vector<string>& values = get<vector<string>>(value_);
        if (values.size() == 1u)
        {
            return VariableValue{ *values.begin() }.Execute(closure, context);
        }

        ObjectHolder obj_holder;
        for (size_t i = 0u; i + 1u < values.size(); ++i)
        {
            if (closure.find(values.at(i)) != closure.end())
            {
                const auto& fields = closure.at(values.at(i)).TryAs<runtime::ClassInstance>()->Fields();
                if (fields.find(values.at(i + 1u)) != fields.end())
                {
                    obj_holder = fields.at(values.at(i + 1u));
                }
                else
                {
                    throw std::runtime_error("");
                }
            }
            else
            {
                throw std::runtime_error("");
            }
        }
        return obj_holder;
    }
    throw std::runtime_error("");
}

unique_ptr<Print> Print::Variable(const std::string& name) {
    return make_unique<Print>(Print{ make_unique<StringConst>(name) });
}

Print::Print(unique_ptr<Statement> argument) 
    :argument_(move(argument)) {}

Print::Print(vector<unique_ptr<Statement>> args) 
    :args_(move(args)) {}

ObjectHolder Print::Execute(Closure& closure, Context& context) {
    ostream& os = context.GetOutputStream();
    if (argument_)
    {
        if (argument_->Execute(closure, context).TryAs<runtime::String>())
        {
            closure.at(argument_->Execute(closure, context).TryAs<runtime::String>()->GetValue())->Print(os, context);
        }
    }
    else if (args_.size() != 0u)
    {
        bool space_flag = false;
        for (const unique_ptr<Statement>& arg : args_) 
        {
            if (space_flag) { os << ' '; }
            space_flag = true;

            ObjectHolder obj_holder = arg->Execute(closure, context);
            if (!obj_holder) 
            {
                os << "None";
                continue;
            }
            obj_holder->Print(os, context);
        }
    }
    os << '\n';
    return {};
}

MethodCall::MethodCall(std::unique_ptr<Statement> object, std::string method,
                       std::vector<std::unique_ptr<Statement>> args) 
    :object_(move(object)), method_(method), args_(move(args)){}

ObjectHolder MethodCall::Execute(Closure& closure, Context& context) 
{
    vector<runtime::ObjectHolder> args;
    for (const auto& arg: args_)
    {
        args.push_back(arg->Execute(closure, context));
    }
    return object_->Execute(closure, context).TryAs<runtime::ClassInstance>()->Call(method_, args, context);
}

ObjectHolder Stringify::Execute(Closure& closure, Context& context) {
    ObjectHolder obj_holder = argument_->Execute(closure, context);
    if (!obj_holder) 
    {
        return ObjectHolder::Own(runtime::String("None"s));
    }
    ostringstream oss;
    obj_holder->Print(oss, context);
    return ObjectHolder::Own(runtime::String(oss.str()));
}

ObjectHolder Add::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs_obj_holder = lhs_->Execute(closure, context);
    ObjectHolder rhs_obj_holder = rhs_->Execute(closure, context);
    ObjectHolder obj_holder;
    if (obj_holder = AddStatements<int,runtime::Number>(lhs_obj_holder, rhs_obj_holder)) 
    {
        return obj_holder;
    }
    else if (obj_holder = AddStatements<string, runtime::String>(lhs_obj_holder, rhs_obj_holder)) 
    {
        return obj_holder;
    }
    else if (lhs_obj_holder.TryAs<runtime::ClassInstance>()) 
    {
        if (lhs_obj_holder.TryAs<runtime::ClassInstance>()->HasMethod(ADD_METHOD, 1U)) 
        {
            return lhs_obj_holder.TryAs<runtime::ClassInstance>()->Call(ADD_METHOD, { rhs_obj_holder }, context);
        }
    }
    throw std::runtime_error("");
}

ObjectHolder Sub::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs_obj_holder = lhs_->Execute(closure, context);
    ObjectHolder rhs_obj_holder = rhs_->Execute(closure, context);
    
    ObjectHolder obj_holder;
    if (lhs_obj_holder.TryAs<runtime::ValueObject<int>>() &&  rhs_obj_holder.TryAs<runtime::ValueObject<int>>()) 
    {
        int lhs = lhs_obj_holder.TryAs<runtime::ValueObject<int>>()->GetValue();
        int rhs = rhs_obj_holder.TryAs<runtime::ValueObject<int>>()->GetValue();
        return runtime::ObjectHolder::Own(runtime::Number(lhs - rhs));
    }
    throw std::runtime_error("");
}

ObjectHolder Mult::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs_obj_holder = lhs_->Execute(closure, context);
    ObjectHolder rhs_obj_holder = rhs_->Execute(closure, context);
    
    ObjectHolder obj_holder;
    if (lhs_obj_holder.TryAs<runtime::ValueObject<int>>() &&  rhs_obj_holder.TryAs<runtime::ValueObject<int>>()) 
    {
        int lhs = lhs_obj_holder.TryAs<runtime::ValueObject<int>>()->GetValue();
        int rhs = rhs_obj_holder.TryAs<runtime::ValueObject<int>>()->GetValue();
        return runtime::ObjectHolder::Own(runtime::Number(lhs * rhs));
    }
    throw std::runtime_error("");
}

ObjectHolder Div::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs_obj_holder = lhs_->Execute(closure, context);
    ObjectHolder rhs_obj_holder = rhs_->Execute(closure, context);
    
    ObjectHolder obj_holder;
    if (lhs_obj_holder.TryAs<runtime::ValueObject<int>>() &&  rhs_obj_holder.TryAs<runtime::ValueObject<int>>()) 
    {
        int lhs = lhs_obj_holder.TryAs<runtime::ValueObject<int>>()->GetValue();
        int rhs = rhs_obj_holder.TryAs<runtime::ValueObject<int>>()->GetValue();

        if (rhs == 0) { throw std::runtime_error(""); }
        return runtime::ObjectHolder::Own(runtime::Number(lhs / rhs));
    }
    throw std::runtime_error("");
}

ObjectHolder Compound::Execute(Closure& closure, Context& context) {
    for (auto& stmt: stmts_)
    {
        if (Return* return_ptr = dynamic_cast<Return*>(stmt.get()))
        {
            return return_ptr->Execute(closure, context);
        }
        if (IfElse* if_else_ptr = dynamic_cast<IfElse*>(stmt.get()))
        {
            if (ObjectHolder obj_holder = if_else_ptr->Execute(closure, context))
            {
                return obj_holder;
            }
        }
        else
        {
            stmt->Execute(closure, context);
        }
    }
    return {};
}

ObjectHolder Return::Execute(Closure& closure, Context& context) {
    return statement_->Execute(closure, context);
}

ClassDefinition::ClassDefinition(ObjectHolder cls) 
    :cls_(cls) {}

ObjectHolder ClassDefinition::Execute(Closure& closure, [[maybe_unused]] Context& context) {
    closure[cls_.TryAs<runtime::Class>()->GetName()] = cls_;
    return cls_;
}

FieldAssignment::FieldAssignment(VariableValue object, std::string field_name,
                                 std::unique_ptr<Statement> rv) 
    :object_(object), field_name_(field_name), rv_(move(rv)) {}

ObjectHolder FieldAssignment::Execute(Closure& closure, Context& context) {
    return object_.Execute(closure, context).TryAs<runtime::ClassInstance>()->Fields()[field_name_] = rv_->Execute(closure, context);
}

IfElse::IfElse(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> if_body,
               std::unique_ptr<Statement> else_body) 
    :condition_(move(condition)), if_body_(move(if_body)), else_body_(move(else_body)) {}

ObjectHolder IfElse::Execute(Closure& closure, Context& context) {
    if (runtime::IsTrue(condition_->Execute(closure, context))) 
    {
        return if_body_->Execute(closure, context); 
    }
    else if (else_body_) 
    {
        return else_body_->Execute(closure, context);
    }
    return {};
}

ObjectHolder Or::Execute(Closure& closure, Context& context) {
    if (runtime::IsTrue(lhs_->Execute(closure, context))) 
    {
        return ObjectHolder::Own(runtime::Bool{ true });
    }
    else if (runtime::IsTrue(rhs_->Execute(closure, context))) 
    {
        return ObjectHolder::Own(runtime::Bool{ true });
    }
    return ObjectHolder::Own(runtime::Bool{ false });
}

ObjectHolder And::Execute(Closure& closure, Context& context) {
    if (runtime::IsTrue(lhs_->Execute(closure, context)) && runtime::IsTrue(rhs_->Execute(closure, context))) 
    {
        return ObjectHolder::Own(runtime::Bool{ true });
    }
    return ObjectHolder::Own(runtime::Bool{ false });
}

ObjectHolder Not::Execute(Closure& closure, Context& context) {
    return ObjectHolder::Own(runtime::Bool{ !runtime::IsTrue(argument_->Execute(closure, context)) });
}

Comparison::Comparison(Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs)
    : BinaryOperation(std::move(lhs), std::move(rhs)), cmp_(cmp) {}

ObjectHolder Comparison::Execute(Closure& closure, Context& context) {
    return ObjectHolder::Own(runtime::Bool{ cmp_(lhs_->Execute(closure, context), rhs_->Execute(closure, context), context) });
}

NewInstance::NewInstance(const runtime::Class& class_, std::vector<std::unique_ptr<Statement>> args)
    :cls_(class_), args_(move(args)) {}

NewInstance::NewInstance(const runtime::Class& cls)
    :cls_(cls) {}

ObjectHolder NewInstance::Execute(Closure& closure, Context& context) {
    ObjectHolder obj_holder = ObjectHolder::Own(runtime::ClassInstance{ cls_ });
    if (const runtime::Method* class_method = cls_.GetMethod(INIT_METHOD); class_method) 
    {
        vector<ObjectHolder> args;
        for (const auto& arg : args_) 
        {
            args.push_back(arg->Execute(closure, context));
        }
        obj_holder.TryAs<runtime::ClassInstance>()->Call(class_method->name, args, context);
    }
    return obj_holder;
}

MethodBody::MethodBody(std::unique_ptr<Statement>&& body) 
    :body_(move(body)) {}

ObjectHolder MethodBody::Execute(Closure& closure, Context& context) {
    return body_->Execute(closure, context);
}

}  // namespace ast