/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2005, 2006, 2007 Ciaran McCreesh <ciaranm@ciaranm.org>
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

#ifndef PALUDIS_GUARD_PALUDIS_DEP_LIST_SHOW_SUGGEST_VISITOR_HH
#define PALUDIS_GUARD_PALUDIS_DEP_LIST_SHOW_SUGGEST_VISITOR_HH 1

#include <paludis/util/private_implementation_pattern.hh>
#include <paludis/dep_list/dep_list-fwd.hh>
#include <paludis/dep_spec-fwd.hh>

namespace paludis
{
    /**
     * Used by DepList to add suggested deps.
     *
     * \ingroup grpdepresolver
     * \nosubgrouping
     */
    class ShowSuggestVisitor :
        public ConstVisitor<DependencySpecTree>,
        public ConstVisitor<DependencySpecTree>::VisitConstSequence<ShowSuggestVisitor, AnyDepSpec>,
        public ConstVisitor<DependencySpecTree>::VisitConstSequence<ShowSuggestVisitor, AllDepSpec>,
        private PrivateImplementationPattern<ShowSuggestVisitor>
    {
        public:
            ///\name Basic operations
            ///\{

            ShowSuggestVisitor(DepList * const dd, tr1::shared_ptr<const DestinationsCollection> ddd,
                    const Environment * const, const PackageDatabaseEntry * const, bool);
            ~ShowSuggestVisitor();

            ///\}

            ///\name Visitor operations
            ///\{

            void visit_leaf(const BlockDepSpec &);

            void visit_leaf(const PackageDepSpec &);

            void visit_sequence(const UseDepSpec &,
                    DependencySpecTree::ConstSequenceIterator,
                    DependencySpecTree::ConstSequenceIterator);
            void visit_sequence(const AnyDepSpec &,
                    DependencySpecTree::ConstSequenceIterator,
                    DependencySpecTree::ConstSequenceIterator);

            using ConstVisitor<DependencySpecTree>::VisitConstSequence<ShowSuggestVisitor, AllDepSpec>::visit_sequence;

            ///\}
    };
}

#endif
