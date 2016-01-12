#include "imgscanner/SaneOption.h"
#include "imgscanner/SaneUtil.h"

#include <sane/sane.h>
#include <ostream>
#include <sstream>
#include <glog/logging.h>

namespace dmscanlib {

namespace imgscanner {

SaneOption::SaneOption(const SANE_Option_Descriptor * optionDesc,
                       const int _optionNumber,
                       const SaneOptionGroup * g) :
      optionNumber(_optionNumber),
      group(g),
      name(optionDesc->name != NULL ? optionDesc->name : ""),
      title(optionDesc->title != NULL ? optionDesc->title : ""),
      desc(optionDesc->desc != NULL ? optionDesc->desc : ""),
      type(optionDesc->type),
      unit(optionDesc->unit),
      size(optionDesc->size),
      cap(optionDesc->cap)
{}

SaneOption::~SaneOption()
{}

int SaneOption::getOptionNumber() const {
   return optionNumber;
}

std::string const & SaneOption::getName() const {
   return name;
}

SANE_Value_Type SaneOption::getType() const {
   return type;
}

SANE_Int SaneOption::getSize() const {
   return size;
}

std::string SaneOption::to_string() const {
   std::stringstream ss;

   ss << "optionNumber: " << optionNumber << std::endl;

   if (!name.empty()) {
      ss << "\tname: " << name << std::endl;
   }

   ss << "\ttitle: " << title << std::endl
      << "\tdesc: " << desc << std::endl
      << "\ttype: " << type
      << ", unit: " << unit
      << ", size: " << size
      << ", cap: " << cap;

   if (group != NULL) {
      ss  << ", group: " << group->title;
   }

   return ss.str();
}

std::string SaneOption::valueAsString(SANE_Handle saneHandle) const {
   std::string result;

   switch (type) {
      case SANE_TYPE_BOOL:
      case SANE_TYPE_INT:
         return std::to_string(SaneUtil::getControlOptionWord(saneHandle, optionNumber));

      case SANE_TYPE_FIXED:
         return std::to_string(
            SANE_UNFIX(SaneUtil::getControlOptionWord(saneHandle, optionNumber)));

      case SANE_TYPE_STRING: {
         SANE_Char value [size];
         SaneUtil::getControlOptionString(saneHandle, optionNumber, value);
         result.append(value);
         break;
      }

      case SANE_TYPE_GROUP:
         // groups do not have value, do nothing
         break;

      default:
         CHECK(false) << "invalid value for option type: " << type;
   }

   return result;
}

SaneOptionGroup::SaneOptionGroup(const SANE_Option_Descriptor * optionDesc,
                                 const int optionNumber) :
      SaneOption(optionDesc, optionNumber, NULL)
{}

SaneOptionGroup::~SaneOptionGroup()
{}

std::string SaneOptionGroup::to_string() const {
   std::stringstream ss;
   ss << "SaneGroup [ title: " << title << " ]";
   return ss.str();
}

template <>
SaneOptionConstraint<int>::SaneOptionConstraint(const SANE_Option_Descriptor * optionDesc,
                                                const int onum,
                                                const SaneOptionGroup * group) :
      SaneOption(optionDesc, onum, group)
{
   int len = optionDesc->constraint.word_list[0];
   for (int j = 1; j <= len; ++j) {
      items.push_back(optionDesc->constraint.word_list[j]);
   }
}


template <>
SaneOptionConstraint<double>::SaneOptionConstraint(const SANE_Option_Descriptor * optionDesc,
                                                   const int onum,
                                                   const SaneOptionGroup * group) :
      SaneOption(optionDesc, onum, group)
{
   int len = optionDesc->constraint.word_list[0];
   for (int j = 1; j <= len; ++j) {
      items.push_back(SANE_UNFIX(optionDesc->constraint.word_list[j]));
   }
}

template <>
SaneOptionConstraint<std::string>::SaneOptionConstraint(const SANE_Option_Descriptor * optionDesc,
                                                        const int onum,
                                                        const SaneOptionGroup * group) :
      SaneOption(optionDesc, onum, group)
{
   int j = 0;
   while (optionDesc->constraint.string_list[j] != NULL) {
      items.push_back(optionDesc->constraint.string_list[j]);
      ++j;
   }
}

template <>
SaneOptionRangeConstraint<int>::SaneOptionRangeConstraint(const SANE_Option_Descriptor * optionDesc,
                                                          const int onum,
                                                          const SaneOptionGroup * group) :
      SaneOption(optionDesc, onum, group),
      min(optionDesc->constraint.range->min),
      max(optionDesc->constraint.range->max),
      quant(optionDesc->constraint.range->quant)
{
}

template <>
SaneOptionRangeConstraint<double>::SaneOptionRangeConstraint(const SANE_Option_Descriptor * optionDesc,
                                                             const int onum,
                                                             const SaneOptionGroup * group) :
      SaneOption(optionDesc, onum, group),
      min(SANE_UNFIX(optionDesc->constraint.range->min)),
      max(SANE_UNFIX(optionDesc->constraint.range->max)),
      quant(SANE_UNFIX(optionDesc->constraint.range->quant))
{
}

std::unique_ptr<SaneOption>
SaneOptionFactory::createOption(const SANE_Handle saneHandle,
                                const int optionNumber,
                                const SaneOptionGroup * group) {
   const SANE_Option_Descriptor * optionDesc = sane_get_option_descriptor(saneHandle, optionNumber);
   CHECK(optionDesc != NULL) << "could not get option description for option " << optionNumber;

   switch (optionDesc->type) {
      case SANE_TYPE_BOOL: {
         switch (optionDesc->constraint_type) {
            case SANE_CONSTRAINT_NONE: {
               return std::unique_ptr<SaneOption>(
                  new SaneOption(optionDesc, optionNumber, group));
            }

            default:
               CHECK(false) << "invalid value for constraint type: " << optionDesc->constraint_type;
         }
         break;
      }

      case SANE_TYPE_INT: {
         SANE_Int len = optionDesc->size / sizeof(SANE_Word);

         if (len == 1) {
            switch (optionDesc->constraint_type) {
               case SANE_CONSTRAINT_NONE: {
                  return std::unique_ptr<SaneOption>(
                     new SaneOption(optionDesc, optionNumber, group));
               }

               case SANE_CONSTRAINT_RANGE: {
                  return std::unique_ptr<SaneOptionRangeConstraint<int>>(
                     new SaneOptionRangeConstraint<int>(optionDesc, optionNumber, group));
               }

               case SANE_CONSTRAINT_WORD_LIST: {
                  return std::unique_ptr<SaneOptionConstraint<int>>(
                     new SaneOptionConstraint<int>(optionDesc, optionNumber, group));
               }

               case SANE_CONSTRAINT_STRING_LIST:
               default:
                  throw std::logic_error("invalid value for constraint type");
                  break;
            }
         } else {
            CHECK(false) << "invalid length for type: " << len;
         }
         break;
      }

      case SANE_TYPE_FIXED: {
         SANE_Int len = optionDesc->size / sizeof(SANE_Word);
         if (len == 1) {
            switch (optionDesc->constraint_type) {
               case SANE_CONSTRAINT_NONE: {
                  return std::unique_ptr<SaneOption>(
                     new SaneOption(optionDesc, optionNumber, group));
                  break;
               }

               case SANE_CONSTRAINT_RANGE: {
                  return std::unique_ptr<SaneOptionRangeConstraint<double>>(
                     new SaneOptionRangeConstraint<double>(optionDesc, optionNumber, group));
                  break;
               }

               case SANE_CONSTRAINT_WORD_LIST: {
                  return std::unique_ptr<SaneOptionConstraint<double>>(
                     new SaneOptionConstraint<double>(optionDesc, optionNumber, group));
               }

               case SANE_CONSTRAINT_STRING_LIST:
               default:
                  CHECK(false) << "invalid value for constraint type: " << optionDesc->constraint_type;
            }
         } else {
            CHECK(false) << "invalid length for type: " << len;
         }
         break;
      }

      case SANE_TYPE_STRING: {
         switch (optionDesc->constraint_type) {
            case SANE_CONSTRAINT_NONE: {
               return std::unique_ptr<SaneOption>(
                  new SaneOption(optionDesc, optionNumber, group));
               break;
            }

            case SANE_CONSTRAINT_STRING_LIST: {
                  return std::unique_ptr<SaneOptionConstraint<std::string>>(
                  new SaneOptionConstraint<std::string>(optionDesc, optionNumber, group));
            }

            case SANE_CONSTRAINT_RANGE:
            case SANE_CONSTRAINT_WORD_LIST:
            default:
               CHECK(false) << "invalid value for constraint type: " << optionDesc->constraint_type;
         }
         break;
      }

      case SANE_TYPE_GROUP: {
         return std::unique_ptr<SaneOptionGroup>(new SaneOptionGroup(optionDesc, optionNumber));
      }

      default:
         break;
   }

   CHECK(false) << "could not create an option";
}

std::ostream & operator<<(std::ostream & os, const SaneOption & o) {
   return os << o.to_string();
}

} /* namespace */

} /* namespace */
