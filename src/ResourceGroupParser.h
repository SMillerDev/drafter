//
//  ResourceGroupParser.h
//  snowcrash
//
//  Created by Zdenek Nemec on 5/4/13.
//  Copyright (c) 2013 Apiary.io. All rights reserved.
//

#ifndef SNOWCRASH_RESOURCEGROUPPARSER_H
#define SNOWCRASH_RESOURCEGROUPPARSER_H

#include "BlueprintParserCore.h"
#include "Blueprint.h"
#include "ResourceParser.h"

namespace snowcrash {
    
    // Finds a group in blueprint by name
    inline Collection<ResourceGroup>::const_iterator FindResourceGroup(const Blueprint& blueprint,
                                                                       const ResourceGroup& group) {

        return std::find_if(blueprint.resourceGroups.begin(),
                            blueprint.resourceGroups.end(),
                            std::bind2nd(MatchName<ResourceGroup>(), group));
    }
        
    //
    // Block Classifier, Resource Group Context
    //
    template <>
    inline Section ClassifyBlock<ResourceGroup>(const BlockIterator& begin,
                                                const BlockIterator& end,
                                                const Section& context) {
        if (begin->type == HRuleBlockType)
            return TerminatorSection;
        
        if (context == TerminatorSection)
            return UndefinedSection;
        
        if (HasResourceSignature(*begin))
            return ResourceSection;
        else if (context == ResourceSection)
            return UndefinedSection;
        else
            return ResourceGroupSection;
    }
    
    //
    // Resource Group Section Parser
    //
    template<>
    struct SectionParser<ResourceGroup> {
        
        static ParseSectionResult ParseSection(const Section& section,
                                               const BlockIterator& cur,
                                               const SectionBounds& bounds,
                                               const BlueprinParserCore& parser,
                                               ResourceGroup& group) {
            
            ParseSectionResult result = std::make_pair(Result(), cur);
            switch (section) {
                case TerminatorSection:
                    if (result.second != bounds.second)
                        ++result.second;
                    break;
                    
                case ResourceGroupSection:
                    result = HandleResourceGroupOverviewBlock(cur, bounds, parser, group);
                    break;
                    
                case ResourceSection:
                    result = HandleResource(cur, bounds.second, parser, group);
                    break;
                    
                case UndefinedSection:
                    break;
                    
                default:
                    result.first.error = Error("unexpected block", 1, cur->sourceMap);
                    break;
            }
            
            return result;
        }
        
        static ParseSectionResult HandleResourceGroupOverviewBlock(const BlockIterator& cur,
                                                                   const SectionBounds& bounds,
                                                                   const BlueprinParserCore& parser,
                                                                   ResourceGroup& group) {
            
            ParseSectionResult result = std::make_pair(Result(), cur);
            BlockIterator sectionCur(cur);
            if (sectionCur->type == HeaderBlockType &&
                sectionCur == bounds.first) {
                group.name = cur->content;
            }
            else {
                if (sectionCur == bounds.first) {
                    
                    // WARN: No Group name specified
                    result.first.warnings.push_back(Warning("expected resource group name, e.g. `# <Group Name>`",
                                                            0,
                                                            cur->sourceMap));
                }
                
                
                if (sectionCur->type == QuoteBlockBeginType) {
                    sectionCur = SkipToSectionEnd(cur, bounds.second, QuoteBlockBeginType, QuoteBlockEndType);
                }
                else if (sectionCur->type == ListBlockBeginType) {
                    sectionCur = SkipToSectionEnd(cur, bounds.second, ListBlockBeginType, ListBlockEndType);
                }
                
                group.description += MapSourceData(parser.sourceData, sectionCur->sourceMap);
            }
            
            result.second = ++sectionCur;
            return result;
        }
        
        static ParseSectionResult HandleResource(const BlockIterator& begin,
                                                 const BlockIterator& end,
                                                 const BlueprinParserCore& parser,
                                                 ResourceGroup& group)
        {
            Resource resource;
            ParseSectionResult result = ResourceParser::Parse(begin, end, parser, resource);
            if (result.first.error.code != Error::OK)
                return result;
            
            ResourceIterator duplicate = FindResource(group, resource);
            ResourceIteratorPair globalDuplicate;
            if (duplicate != group.resources.end())
                globalDuplicate = FindResource(parser.blueprint, resource);
            
            if (duplicate != group.resources.end() ||
                globalDuplicate.first != parser.blueprint.resourceGroups.end()) {
                
                // WARN: duplicate resource
                result.first.warnings.push_back(Warning("resource `" +
                                                        resource.uri +
                                                        "` is already defined",
                                                        0,
                                                        begin->sourceMap));
            }
            
            group.resources.push_back(resource); // FIXME: C++11 move
            return result;
        }
    };
    
    typedef BlockParser<ResourceGroup, SectionParser<ResourceGroup> > ResourceGroupParser;
}

#endif
