/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2007 David Leverton <levertond@googlemail.com>
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

#ifndef PALUDIS_GUARD_PALUDIS_DEP_LIST_CONDITION_TRACKER_HH
#define PALUDIS_GUARD_PALUDIS_DEP_LIST_CONDITION_TRACKER_HH 1

#include <paludis/util/tr1_memory.hh>
#include <paludis/util/tr1_functional.hh>
#include <paludis/util/visitor.hh>
#include <paludis/dep_tree.hh>
#include <paludis/dep_spec-fwd.hh>

namespace paludis
{
    class ConditionTracker :
        public ConstVisitor<DependencySpecTree>,
        public ConstVisitor<DependencySpecTree>::VisitConstSequence<ConditionTracker, AllDepSpec>
    {
        private:
            tr1::shared_ptr<ConstTreeSequence<DependencySpecTree, AllDepSpec> > base;
            tr1::function<void (tr1::shared_ptr<ConstAcceptInterface<DependencySpecTree> >)> adder;

            template <typename T_>
            void do_visit_sequence(const T_ &,
                        DependencySpecTree::ConstSequenceIterator,
                        DependencySpecTree::ConstSequenceIterator);

            template <typename T_>
            tr1::shared_ptr<ConstTreeSequence<DependencySpecTree, AllDepSpec> > do_add_sequence(const T_ &);
            template <typename T_>
            tr1::shared_ptr<ConstTreeSequence<DependencySpecTree, AllDepSpec> > do_add_leaf(const T_ &);

        public:
            ConditionTracker(tr1::shared_ptr<ConstTreeSequence<DependencySpecTree, AllDepSpec> >);

            virtual ~ConditionTracker();

            tr1::shared_ptr<ConstTreeSequence<DependencySpecTree, AllDepSpec> > add_condition(const AnyDepSpec &);
            tr1::shared_ptr<ConstTreeSequence<DependencySpecTree, AllDepSpec> > add_condition(const UseDepSpec &);
            tr1::shared_ptr<ConstTreeSequence<DependencySpecTree, AllDepSpec> > add_condition(const PackageDepSpec &);
            tr1::shared_ptr<ConstTreeSequence<DependencySpecTree, AllDepSpec> > add_condition(const BlockDepSpec &);

            using ConstVisitor<DependencySpecTree>::VisitConstSequence<ConditionTracker, AllDepSpec>::visit_sequence;

            void visit_sequence(const AnyDepSpec &, DependencySpecTree::ConstSequenceIterator, DependencySpecTree::ConstSequenceIterator);
            void visit_sequence(const UseDepSpec &, DependencySpecTree::ConstSequenceIterator, DependencySpecTree::ConstSequenceIterator);

            void visit_leaf(const PackageDepSpec &) PALUDIS_ATTRIBUTE((noreturn));
            void visit_leaf(const BlockDepSpec &) PALUDIS_ATTRIBUTE((noreturn));
            void visit_leaf(const LabelsDepSpec<DependencyLabelVisitorTypes> &) PALUDIS_ATTRIBUTE((noreturn));
    };
}

#endif
