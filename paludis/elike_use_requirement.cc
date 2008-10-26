/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2008 Ciaran McCreesh
 *
 * This file is part of the Paludis package manager. Paludis is free software;
 * you can redistribute it and/or modify it under the terms of the GNU General
 * Public License version 2, as published by the Free Software Foundation.
 *
 * Paludis is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <paludis/elike_use_requirement.hh>
#include <paludis/util/options.hh>
#include <paludis/util/make_shared_ptr.hh>
#include <paludis/util/stringify.hh>
#include <paludis/util/join.hh>
#include <paludis/util/tokeniser.hh>
#include <paludis/util/log.hh>
#include <paludis/util/wrapped_forward_iterator.hh>
#include <paludis/util/tribool.hh>
#include <paludis/dep_spec.hh>
#include <paludis/name.hh>
#include <paludis/environment.hh>
#include <paludis/package_id.hh>
#include <paludis/metadata_key.hh>
#include <paludis/choice.hh>
#include <vector>
#include <functional>
#include <algorithm>

using namespace paludis;

namespace
{
    bool icky_use_query(const ChoiceNameWithPrefix & f, const PackageID & id, Tribool default_value = Tribool(indeterminate))
    {
        if (! id.choices_key())
        {
            Log::get_instance()->message("elike_use_requirement.query", ll_warning, lc_context) <<
                "ID '" << id << "' has no choices, so couldn't get the state of flag '" << f << "'";
            return false;
        }

        const std::tr1::shared_ptr<const ChoiceValue> v(id.choices_key()->value()->find_by_name_with_prefix(f));
        if (v)
            return v->enabled();

        if (default_value.is_indeterminate() && ! id.choices_key()->value()->has_matching_contains_every_value_prefix(f))
            Log::get_instance()->message("elike_use_requirement.query", ll_warning, lc_context) <<
                "ID '" << id << "' has no flag named '" << f << "'";
        return default_value.is_true();
    }

    class UseRequirement
    {
        private:
            const ChoiceNameWithPrefix _name;
            const Tribool _default_value;

        public:
            UseRequirement(const ChoiceNameWithPrefix & n, Tribool d) :
                _name(n),
                _default_value(d)
            {
            }
            virtual ~UseRequirement() { }

            const ChoiceNameWithPrefix flag() const PALUDIS_ATTRIBUTE((warn_unused_result))
            {
                return _name;
            }

            const Tribool default_value() const PALUDIS_ATTRIBUTE((warn_unused_result))
            {
                return _default_value;
            }

            virtual bool requirement_met(const Environment * const, const PackageID &) const PALUDIS_ATTRIBUTE((warn_unused_result)) = 0;
            virtual const std::string as_human_string() const PALUDIS_ATTRIBUTE((warn_unused_result)) = 0;

            const std::string default_value_human_string_fragment() const PALUDIS_ATTRIBUTE((warn_unused_result))
            {
                if (_default_value.is_true())
                    return ", assuming enabled if missing";
                else if (_default_value.is_false())
                    return ", assuming disabled if missing";
                else
                    return "";
            }
    };

    class PALUDIS_VISIBLE EnabledUseRequirement :
        public UseRequirement
    {
        public:
            EnabledUseRequirement(const ChoiceNameWithPrefix & n, Tribool d) :
                UseRequirement(n, d)
            {
            }

            virtual bool requirement_met(const Environment * const, const PackageID & pkg) const
            {
                return icky_use_query(flag(), pkg, default_value());
            }

            virtual const std::string as_human_string() const
            {
                return "Flag '" + stringify(flag()) + "' enabled" + default_value_human_string_fragment();
            }
    };

    class PALUDIS_VISIBLE DisabledUseRequirement :
        public UseRequirement
    {
        public:
            DisabledUseRequirement(const ChoiceNameWithPrefix & n, Tribool d) :
                UseRequirement(n, d)
            {
            }

            virtual bool requirement_met(const Environment * const, const PackageID & pkg) const
            {
                return ! icky_use_query(flag(), pkg, default_value());
            }

            virtual const std::string as_human_string() const
            {
                return "Flag '" + stringify(flag()) + "' disabled" + default_value_human_string_fragment();
            }
    };

    class PALUDIS_VISIBLE ConditionalUseRequirement :
        public UseRequirement
    {
        private:
            const std::tr1::shared_ptr<const PackageID> _id;

        public:
            ConditionalUseRequirement(const ChoiceNameWithPrefix & n, const std::tr1::shared_ptr<const PackageID> & i, Tribool d) :
                UseRequirement(n, d),
                _id(i)
            {
            }

            const std::tr1::shared_ptr<const PackageID> package_id() const PALUDIS_ATTRIBUTE((warn_unused_result))
            {
                return _id;
            }
    };

    class PALUDIS_VISIBLE IfMineThenUseRequirement :
        public ConditionalUseRequirement
    {
        public:
            IfMineThenUseRequirement(const ChoiceNameWithPrefix & n, const std::tr1::shared_ptr<const PackageID> & i, Tribool d) :
                ConditionalUseRequirement(n, i, d)
            {
            }

            virtual bool requirement_met(const Environment * const, const PackageID & pkg) const
            {
                return ! icky_use_query(flag(), *package_id()) || icky_use_query(flag(), pkg, default_value());
            }

            virtual const std::string as_human_string() const
            {
                return "Flag '" + stringify(flag()) + "' enabled if it is enabled for '" + stringify(*package_id()) + "'" + default_value_human_string_fragment();
            }
    };

    class PALUDIS_VISIBLE IfNotMineThenUseRequirement :
        public ConditionalUseRequirement
    {
        public:
            IfNotMineThenUseRequirement(const ChoiceNameWithPrefix & n, const std::tr1::shared_ptr<const PackageID> & i, Tribool d) :
                ConditionalUseRequirement(n, i, d)
            {
            }

            virtual bool requirement_met(const Environment * const, const PackageID & pkg) const
            {
                return icky_use_query(flag(), *package_id()) || icky_use_query(flag(), pkg, default_value());
            }

            virtual const std::string as_human_string() const
            {
                return "Flag '" + stringify(flag()) + "' enabled if it is disabled for '" + stringify(*package_id()) + "'" + default_value_human_string_fragment();
            }
    };

    class PALUDIS_VISIBLE IfMineThenNotUseRequirement :
        public ConditionalUseRequirement
    {
        public:
            IfMineThenNotUseRequirement(const ChoiceNameWithPrefix & n, const std::tr1::shared_ptr<const PackageID> & i, Tribool d) :
                ConditionalUseRequirement(n, i, d)
            {
            }

            virtual bool requirement_met(const Environment * const, const PackageID & pkg) const
            {
                return ! icky_use_query(flag(), *package_id()) || ! icky_use_query(flag(), pkg, default_value());
            }

            virtual const std::string as_human_string() const
            {
                return "Flag '" + stringify(flag()) + "' disabled if it is enabled for '" + stringify(*package_id()) + "'" + default_value_human_string_fragment();
            }
    };

    class PALUDIS_VISIBLE IfNotMineThenNotUseRequirement :
        public ConditionalUseRequirement
    {
        public:
            IfNotMineThenNotUseRequirement(const ChoiceNameWithPrefix & n, const std::tr1::shared_ptr<const PackageID> & i, Tribool d) :
                ConditionalUseRequirement(n, i, d)
            {
            }

            virtual bool requirement_met(const Environment * const, const PackageID & pkg) const
            {
                return icky_use_query(flag(), *package_id()) || ! icky_use_query(flag(), pkg, default_value());
            }

            virtual const std::string as_human_string() const
            {
                return "Flag '" + stringify(flag()) + "' disabled if it is disabled for '" + stringify(*package_id()) + "'" + default_value_human_string_fragment();
            }
    };

    class PALUDIS_VISIBLE EqualUseRequirement :
        public ConditionalUseRequirement
    {
        public:
            EqualUseRequirement(const ChoiceNameWithPrefix & n, const std::tr1::shared_ptr<const PackageID> & i, Tribool d) :
                ConditionalUseRequirement(n, i, d)
            {
            }

            virtual bool requirement_met(const Environment * const, const PackageID & pkg) const
            {
                return icky_use_query(flag(), pkg, default_value()) == icky_use_query(flag(), *package_id());
            }

            virtual const std::string as_human_string() const
            {
                return "Flag '" + stringify(flag()) + "' enabled or disabled like it is for '" + stringify(*package_id()) + "'" + default_value_human_string_fragment();
            }
    };

    class PALUDIS_VISIBLE NotEqualUseRequirement :
        public ConditionalUseRequirement
    {
        public:
            NotEqualUseRequirement(const ChoiceNameWithPrefix & n, const std::tr1::shared_ptr<const PackageID> & i, Tribool d) :
                ConditionalUseRequirement(n, i, d)
            {
            }

            virtual bool requirement_met(const Environment * const, const PackageID & pkg) const
            {
                return icky_use_query(flag(), pkg, default_value()) != icky_use_query(flag(), *package_id());
            }

            virtual const std::string as_human_string() const
            {
                return "Flag '" + stringify(flag()) + "' enabled or disabled opposite to how it is for '" + stringify(*package_id()) + "'" + default_value_human_string_fragment();
            }
    };

    class UseRequirements :
        public AdditionalPackageDepSpecRequirement
    {
        private:
            std::string _raw;
            std::vector<std::tr1::shared_ptr<const UseRequirement> > _reqs;

        public:
            UseRequirements(const std::string & r) :
                _raw(r)
            {
            }

            virtual bool requirement_met(const Environment * const env, const PackageID & id) const
            {
                using namespace std::tr1::placeholders;
                return _reqs.end() == std::find_if(_reqs.begin(), _reqs.end(), std::tr1::bind(
                        std::logical_not<bool>(), std::tr1::bind(
                             &UseRequirement::requirement_met, _1, env, std::tr1::cref(id))));
            }

            virtual const std::string as_human_string() const
            {
                return join(_reqs.begin(), _reqs.end(), "; ", std::tr1::mem_fn(&UseRequirement::as_human_string));
            }

            virtual const std::string as_raw_string() const
            {
                return _raw;
            }

            void add_requirement(const std::tr1::shared_ptr<const UseRequirement> & req)
            {
                _reqs.push_back(req);
            }
    };

    template <typename T_>
    std::tr1::shared_ptr<const UseRequirement>
    make_unconditional_requirement(const ChoiceNameWithPrefix & n, const std::tr1::shared_ptr<const PackageID > &, Tribool d)
    {
        return make_shared_ptr(new T_(n, d));
    }

    template <typename T_>
    std::tr1::shared_ptr<const UseRequirement>
    make_conditional_requirement(const ChoiceNameWithPrefix & n, const std::tr1::shared_ptr<const PackageID > & i, Tribool d)
    {
        return make_shared_ptr(new T_(n, i, d));
    }

    std::tr1::shared_ptr<const UseRequirement>
    parse_one_use_requirement(const std::string & s, std::string & flag,
            const std::tr1::shared_ptr<const PackageID> & id, const ELikeUseRequirementOptions & options)
    {
        typedef std::tr1::shared_ptr<const UseRequirement> (* Factory)(const ChoiceNameWithPrefix &, const std::tr1::shared_ptr<const PackageID> &, Tribool);
        Factory factory;

        if (flag.empty())
            throw ELikeUseRequirementError(s, "Invalid [] contents");

        if ('=' == flag.at(flag.length() - 1))
        {
            if ((! options[euro_allow_self_deps]) || (! id))
                throw ELikeUseRequirementError(s, "Cannot use [use=] here");

            flag.erase(flag.length() - 1);
            if (flag.empty())
                throw ELikeUseRequirementError(s, "Invalid [] contents");

            if ('!' == flag.at(flag.length() - 1))
            {
                if (options[euro_portage_syntax] && ! options[euro_both_syntaxes])
                {
                    if (options[euro_strict_parsing])
                        throw ELikeUseRequirementError(s, "[use!=] not safe for use here");
                    else
                        Log::get_instance()->message("e.use_requirement.flag_not_equal_not_allowed", ll_warning, lc_context)
                            << "[use!=] not safe for use here";
                }

                flag.erase(flag.length() - 1, 1);
                if (flag.empty())
                    throw ELikeUseRequirementError(s, "Invalid [] contents");
                factory = make_conditional_requirement<NotEqualUseRequirement>;
            }
            else if ('!' == flag.at(0))
            {
                if (! options[euro_portage_syntax] && ! options[euro_both_syntaxes])
                {
                    if (options[euro_strict_parsing])
                        throw ELikeUseRequirementError(s, "[!use=] not safe for use here");
                    else
                        Log::get_instance()->message("e.use_requirement.not_flag_equal_not_allowed", ll_warning, lc_context)
                            << "[!use=] not safe for use here";
                }

                flag.erase(0, 1);
                if (flag.empty())
                    throw ELikeUseRequirementError(s, "Invalid [] contents");
                factory = make_conditional_requirement<NotEqualUseRequirement>;
            }
            else
                factory = make_conditional_requirement<EqualUseRequirement>;
        }
        else if ('?' == flag.at(flag.length() - 1))
        {
            if ((! options[euro_allow_self_deps]) || (! id))
                throw ELikeUseRequirementError(s, "Cannot use [use?] here");

            flag.erase(flag.length() - 1);
            if (flag.empty())
                throw ELikeUseRequirementError(s, "Invalid [] contents");

            if ('!' == flag.at(flag.length() - 1))
            {
                flag.erase(flag.length() - 1, 1);
                if (flag.empty())
                    throw ELikeUseRequirementError(s, "Invalid [] contents");

                if ('-' == flag.at(0))
                {
                    if (options[euro_portage_syntax] && ! options[euro_both_syntaxes])
                    {
                        if (options[euro_strict_parsing])
                            throw ELikeUseRequirementError(s, "[-use!?] not safe for use here");
                        else
                            Log::get_instance()->message("e.use_requirement.minus_flag_not_question_not_allowed", ll_warning, lc_context)
                                << "[-use!?] not safe for use here";
                    }

                    flag.erase(0, 1);
                    if (flag.empty())
                        throw ELikeUseRequirementError(s, "Invalid [] contents");

                    factory = make_conditional_requirement<IfNotMineThenNotUseRequirement>;
                }
                else
                {
                    if (options[euro_portage_syntax] && ! options[euro_both_syntaxes])
                    {
                        if (options[euro_strict_parsing])
                            throw ELikeUseRequirementError(s, "[use!?] not safe for use here");
                        else
                            Log::get_instance()->message("e.use_requirement.flag_not_question_not_allowed", ll_warning, lc_context)
                                << "[use!?] not safe for use here";
                    }

                    factory = make_conditional_requirement<IfNotMineThenUseRequirement>;
                }
            }
            else if ('!' == flag.at(0))
            {
                if (! options[euro_portage_syntax] && ! options[euro_both_syntaxes])
                {
                    if (options[euro_strict_parsing])
                        throw ELikeUseRequirementError(s, "[!use?] not safe for use here");
                    else
                        Log::get_instance()->message("e.use_requirement.not_flag_question_not_allowed", ll_warning, lc_context)
                            << "[!use?] not safe for use here";
                }

                flag.erase(0, 1);
                if (flag.empty())
                    throw ELikeUseRequirementError(s, "Invalid [] contents");
                factory = make_conditional_requirement<IfNotMineThenNotUseRequirement>;
            }
            else
            {
                if ('-' == flag.at(0))
                {
                    if (options[euro_portage_syntax] && ! options[euro_both_syntaxes])
                    {
                        if (options[euro_strict_parsing])
                            throw ELikeUseRequirementError(s, "[-use?] not safe for use here");
                        else
                            Log::get_instance()->message("e.use_requirement.minus_flag_question_not_allowed", ll_warning, lc_context)
                                << "[-use?] not safe for use here";
                    }

                    flag.erase(0, 1);
                    if (flag.empty())
                        throw ELikeUseRequirementError(s, "Invalid [] contents");

                    factory = make_conditional_requirement<IfMineThenNotUseRequirement>;
                }
                else
                    factory = make_conditional_requirement<IfMineThenUseRequirement>;
            }
        }
        else if ('-' == flag.at(0))
        {
            flag.erase(0, 1);
            if (flag.empty())
                throw ELikeUseRequirementError(s, "Invalid [] contents");
            factory = make_unconditional_requirement<DisabledUseRequirement>;
        }
        else
            factory = make_unconditional_requirement<EnabledUseRequirement>;

        if (')' == flag.at(flag.length() - 1))
        {
            if (flag.length() < 4 || flag.at(flag.length() - 3) != '(')
                throw ELikeUseRequirementError(s, "Invalid [] contents");

            if ('+' == flag.at(flag.length() - 2))
            {
                if (! options[euro_allow_default_values])
                {
                    if (options[euro_strict_parsing])
                        throw ELikeUseRequirementError(s, "[use(+)] not safe for use here");
                    else
                        Log::get_instance()->message("e.use_requirement.flag_bracket_plus_not_allowed", ll_warning, lc_context)
                            << "[use(+)] not safe for use here";
                }

                flag.erase(flag.length() - 3, 3);
                return factory(ChoiceNameWithPrefix(flag), id, Tribool(true));
            }
            else if ('-' == flag.at(flag.length() - 2))
            {
                if (! options[euro_allow_default_values])
                {
                    if (options[euro_strict_parsing])
                        throw ELikeUseRequirementError(s, "[use(-)] not safe for use here");
                    else
                        Log::get_instance()->message("e.use_requirement.flag_bracket_minus_not_allowed", ll_warning, lc_context)
                            << "[use(-)] not safe for use here";
                }

                flag.erase(flag.length() - 3, 3);
                return factory(ChoiceNameWithPrefix(flag), id, Tribool(false));
            }
            else
                throw ELikeUseRequirementError(s, "Invalid [] contents");
        }
        else
            return factory(ChoiceNameWithPrefix(flag), id, Tribool(indeterminate));
    }
}
ELikeUseRequirementError::ELikeUseRequirementError(const std::string & s, const std::string & m) throw () :
    Exception("Error parsing use requirement '" + s + "': " + m)
{
}

std::tr1::shared_ptr<const AdditionalPackageDepSpecRequirement>
paludis::parse_elike_use_requirement(const std::string & s,
        const std::tr1::shared_ptr<const PackageID> & id, const ELikeUseRequirementOptions & options)
{
    Context context("When parsing use requirement '" + s + "':");

    std::tr1::shared_ptr<UseRequirements> result(new UseRequirements("[" + s + "]"));
    std::string::size_type pos(0);
    for (;;)
    {
        std::string::size_type comma(s.find(',', pos));
        std::string flag(s.substr(pos, std::string::npos == comma ? comma : comma - pos));
        result->add_requirement(parse_one_use_requirement(s, flag, id, options));
        if (std::string::npos == comma)
            break;

        if (! options[euro_portage_syntax] && ! options[euro_both_syntaxes])
        {
            if (options[euro_strict_parsing])
                throw ELikeUseRequirementError(s, "[use,use] not safe for use here");
            else
                Log::get_instance()->message("e.use_requirement.comma_separated_not_allowed", ll_warning, lc_context)
                    << "[use,use] not safe for use here";
        }

        pos = comma + 1;
    }

    return result;
}

