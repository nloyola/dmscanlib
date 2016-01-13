#ifndef __INCLUDE_SANE_OPTION_H
#define __INCLUDE_SANE_OPTION_H

#include <sane/sane.h>
#include <vector>
#include <utility>
#include <memory>
#include <ostream>
#include <sstream>

namespace dmscanlib {

namespace imgscanner {

class SaneOptionGroup;

class SaneOption {
public:
   SaneOption(const SANE_Option_Descriptor * optionDesc,
              const int optionNumber,
              const SaneOptionGroup * g);

   virtual ~SaneOption();

   virtual std::string to_string() const;

   std::string const & getName() const;

   SANE_Value_Type getType() const;

   SANE_Int getOptionNumber() const;

   SANE_Int getSize() const;

   std::string valueAsString(SANE_Handle saneHandle) const;

protected:
   friend std::ostream & operator<<(std::ostream & os, const SaneOption & o);

   const int optionNumber;
   const SaneOptionGroup * group;
   const std::string name;
   const std::string title;
   const std::string desc;
   const SANE_Value_Type type;
   const SANE_Unit unit;
   const int size;
   const int cap;
};

class SaneOptionGroup: public SaneOption {
public:
   SaneOptionGroup(const SANE_Option_Descriptor * optionDesc, const int optionNumber);

   virtual ~SaneOptionGroup();

   virtual std::string to_string() const;
};

template<typename T>
class SaneOptionConstraint: public SaneOption {
public:
   SaneOptionConstraint(const SANE_Option_Descriptor * optionDesc,
                        const int optionNumber,
                        const SaneOptionGroup * group);

   virtual ~SaneOptionConstraint() {}

   virtual std::string to_string() const {
      std::stringstream ss;
      ss << "SaneOptionConstraint [ " << SaneOption::to_string();
      if (items.size() > 0) {
         ss << std::endl << "\tconstraints: [ ";
         for (auto item : items) {
            ss << item << " ";
         }
         ss << "]";
      }
      ss << " ]";
      return ss.str();
   }

   bool contains(T item) const {
      return (std::find(items.begin(), items.end(), item) != items.end());
   }

   void copyItems(std::vector<T> & toVector) const {
      toVector = items;
   }

private:
   std::vector<T> items;
};

template<class T>
class SaneOptionRangeConstraint: public SaneOption {
public:
   SaneOptionRangeConstraint(const SANE_Option_Descriptor * optionDesc,
                             const int optionNumber,
                             const SaneOptionGroup * group);

   virtual ~SaneOptionRangeConstraint() {}

   virtual std::string to_string() const {
      std::stringstream ss;
      ss << "SaneOptionRangeConstraint [ " << SaneOption::to_string()
         << std::endl
         << "\tmin: " << min
         << ", max: " << max
         << ", quant: " << quant
         << " ]";
      return ss.str();
   }

   bool validValue(T value) const {
      return ((value >= min) && (value <= max));
   }

   const T getMin() const { return min; }
   const T getMax() const { return max; }
   const T getQuant() const { return quant; }

private:
   const T min;
   const T max;
   const T quant;
};

class SaneOptionFactory {
public:
   static std::unique_ptr<SaneOption> createOption(const SANE_Handle saneHandle,
                                                   const int optionNumber,
                                                   const SaneOptionGroup * group);

};

} /* namespace */

} /* namespace */

/* Local Variables: */
/* mode: c++        */
/* End:             */

#endif /* __INCLUDE_SANE_OPTION_H */
