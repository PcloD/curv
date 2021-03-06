// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#ifndef CURV_DEFINITION_H
#define CURV_DEFINITION_H

#include <curv/analyzer.h>

namespace curv {

struct Scope;
struct Abstract_Definition : public Shared_Base
{
    Shared<const Phrase> source_;
    enum Kind { k_recursive, k_sequential } kind_;

    Abstract_Definition(
        Shared<const Phrase> source,
        Kind k)
    :
        source_(std::move(source)),
        kind_(k)
    {}

    virtual void add_to_scope(Scope&) = 0;
};
// A unitary definition is one that occupies a single node in the dependency
// graph that is constructed while analyzing a recursive scope. It can define
// multiple names.
struct Unitary_Definition : public Abstract_Definition
{
    Unitary_Definition(
        Shared<const Phrase> source,
        Kind k)
    :
        Abstract_Definition(source, k)
    {}

    virtual void analyze(Environ&) = 0;
    virtual Shared<Operation> make_action(slot_t module_slot) = 0;
};
// A function definition is `f=<lambda>` or `f x=<expr>`.
// Only `=` style function definitions belong to this class, so it's potentially
// a recursive definition.
// Only a single name is bound. There is no pattern matching in the definiendum.
struct Function_Definition : public Unitary_Definition
{
    Shared<const Identifier> name_;
    Shared<Lambda_Phrase> definiens_phrase_;
    slot_t slot_; // initialized by add_to_scope()
    Shared<Lambda_Expr> definiens_expr_; // initialized by analyze()

    Function_Definition(
        Shared<const Phrase> source,
        Shared<const Identifier> name,
        Shared<Lambda_Phrase> definiens)
    :
        Unitary_Definition(source, k_recursive),
        name_(std::move(name)),
        definiens_phrase_(std::move(definiens))
    {}

    virtual void add_to_scope(Scope&) override;
    virtual void analyze(Environ&) override;
    virtual Shared<Operation> make_action(slot_t module_slot) override;
};
// A data definition is `pattern = expression` or `var pattern := expression`
// or a `var f x:=expr` style function definition.
// Data definitions cannot be recursive, and they can use pattern matching
// to bind multiple names.
struct Data_Definition : public Unitary_Definition
{
    Shared<const Identifier> name_;
    Shared<Phrase> definiens_phrase_;
    slot_t slot_; // initialized by add_to_scope()
    Shared<Operation> definiens_expr_; // initialized by analyze()

    Data_Definition(
        Shared<const Phrase> source,
        Kind k,
        Shared<const Identifier> name,
        Shared<Phrase> definiens)
    :
        Unitary_Definition(source, k),
        name_(std::move(name)),
        definiens_phrase_(std::move(definiens))
    {}

    virtual void add_to_scope(Scope&) override;
    virtual void analyze(Environ&) override;
    virtual Shared<Operation> make_action(slot_t module_slot) override;
};
// A compound definition is `statement1;statement2;...` where each statement
// is an action or definition, and there is at least one definition.
struct Compound_Definition_Base : public Abstract_Definition
{
    struct Element
    {
        Shared<const Phrase> phrase_;
        Shared<Abstract_Definition> definition_;
    };

    Compound_Definition_Base(Shared<const Phrase> source)
    : Abstract_Definition(std::move(source), k_recursive) {}

    virtual void add_to_scope(Scope&) override;

    TAIL_ARRAY_MEMBERS(Element)
};
using Compound_Definition = aux::Tail_Array<Compound_Definition_Base>;
struct Scope
{
    virtual void analyze(Abstract_Definition&) = 0;
    virtual void add_action(Shared<const Phrase>) = 0;
    virtual unsigned begin_unit(Shared<Unitary_Definition>) = 0;
    virtual slot_t add_binding(Shared<const Identifier>, unsigned unit) = 0;
    virtual void end_unit(unsigned, Shared<Unitary_Definition>) = 0;
};
struct Sequential_Scope : public Scope, public Environ
{
    bool target_is_module_;
    Shared<Module::Dictionary> dictionary_ = make<Module::Dictionary>();
    Scope_Executable executable_ = {};

    Sequential_Scope(Environ& parent, bool target_is_module)
    :
        Environ(&parent),
        target_is_module_(target_is_module)
    {
        frame_nslots_ = parent.frame_nslots_;
        frame_maxslots_ = parent.frame_maxslots_;
        is_sequential_statement_list_ = true;
        if (target_is_module)
            executable_.module_slot_ = make_slot();
    }

    virtual Shared<Meaning> single_lookup(const Identifier&) override;
    virtual void analyze(Abstract_Definition&) override;
    virtual void add_action(Shared<const Phrase>) override;
    virtual unsigned begin_unit(Shared<Unitary_Definition>) override;
    virtual slot_t add_binding(Shared<const Identifier>, unsigned unit) override;
    virtual void end_unit(unsigned, Shared<Unitary_Definition>) override;
};

} // namespace
#endif // header guard
