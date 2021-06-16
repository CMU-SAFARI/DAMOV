#ifndef __STATISTICS_H
#define __STATISTICS_H

#include <string>
#include "StatType.h"

/*
  IMPORTANT NOTE - Read this first!

  This version of the file provides wrappers to the gem5 statistics classes.
  Feel free to go through this file, though it can be difficult to follow
  with the degree of abstraction going on. In short, this file currently
  provides the following mapping of stat classes. In almost all cases, the
  wrapper provides identical and complete functionality to the gem5 stat
  classes. All of our classes are defined in the ramulator namespace.

  GEM5 CLASS --> RAMULATOR CLASS
  ==============================
  Stats::Scalar --> ScalarStat
  Stats::Average --> AverageStat
  Stats::Vector --> VectorStat
  Stats::AverageVector --> AverageVectorStat
  Stats::Distribution --> DistributionStat
  Stats::Histogram --> HistogramStat
  Stats::StandardDeviation --> StandardDeviationStat
  Stats::AverageDeviation --> AverageDeviationStat

  All of the stats that you create will be named "ramulator.<your name>"
  automatically, and will be dumped at the end of simulation into the gem5
  stats file.
*/

namespace ramulator {

template<class StatType>
class StatBase { // wrapper for Stats::DataWrap
  protected:
    StatType stat;
    std::string statName;

    StatBase<StatType> & self() { return *this; }
  public:
    StatBase() {}

#ifndef INTEGRATED_WITH_GEM5
    const StatType* get_stat() const {
      return &stat;
    }
#endif

    StatBase(std::string _name) {
      name(_name);
    }

    StatBase(std::string _name, std::string _desc) {
      name(_name);
      desc(_desc);
    }

    StatBase<StatType> & name(std::string _name) {
      statName = _name;
      stat.name("ramulator." + _name);

      return self();
    }

    const std::string &name(void) const { return statName; }

    StatBase<StatType> & setSeparator(const std::string & _sep) {
      stat.setSeparator(_sep);
      return self();
    }

    const std::string &setSeparator() const { return stat.setSeparator(); }

    StatBase<StatType> & desc(std::string _desc) {
      stat.desc(_desc);
      return self();
    }

    StatBase<StatType> & precision(int _precision) {
      stat.precision(_precision);
      return self();
    }

    StatBase<StatType> & flags(Stats_ramulator::Flags _flags) {
      stat.flags(_flags);
      return self();
    }

    template <class Stat>
    StatBase<StatType> & prereq(const Stat & _prereq) {
      stat.prereq(_prereq);
      return self();
    }

    Stats_ramulator::size_type size(void) const { return stat.size(); }
    bool zero(void) const { return stat.zero(); }
    void prepare(void) { stat.prepare(); }
    void reset(void) { stat.reset(); }
};

template<class StatType>
class StatBaseVec : public StatBase<StatType> { // wrapper for Stats::DataWrapVec
  protected:
    StatBaseVec<StatType> & self() { return *this; }

  public:
    StatBaseVec<StatType> & subname(Stats_ramulator::off_type index, const std::string & name) {
      StatBase<StatType>::stat.subname(index, name);
      return self();
    }

    StatBaseVec<StatType> & subdesc(Stats_ramulator::off_type index, const std::string & desc) {
      StatBase<StatType>::stat.subdesc(index, desc);
      return self();
    }
};

template<class StatType>
class ScalarStatBase : public StatBase<StatType> { // wrapper for Stats::ScalarBase
  public:
    Stats_ramulator::Counter value() const { return StatBase<StatType>::stat.value(); };
    void operator++() { ++StatBase<StatType>::stat; }
    void operator--() { --StatBase<StatType>::stat; }

    void operator++(int) { StatBase<StatType>::stat++; }
    void operator--(int) { StatBase<StatType>::stat--; }

    template <typename U>
    void operator=(const U &v) { StatBase<StatType>::stat = v; }

    template <typename U>
    void operator+=(const U &v) { StatBase<StatType>::stat += v; }

    template <typename U>
    void operator-=(const U &v) { StatBase<StatType>::stat -= v; }
};

template<class StatType, class Element>
class VectorStatBase : public StatBaseVec<StatType> { // wrapper for Stats::VectorBase
  protected:
    VectorStatBase<StatType, Element> & self() { return *this; }

  public:
    void value(Stats_ramulator::VCounter & vec) const { StatBase<StatType>::stat.value(vec); }
    void result(Stats_ramulator::VResult & vec) const { StatBase<StatType>::stat.result(vec); }
    Stats_ramulator::Result total(void) const { return StatBase<StatType>::stat.total(); }

    bool check(void) const { return StatBase<StatType>::stat.check(); }

    VectorStatBase<StatType, Element> & init(Stats_ramulator::size_type size) {
      StatBase<StatType>::stat.init(size);
      return self();
    }
    Element &operator[](Stats_ramulator::off_type index) { return StatBase<StatType>::stat[index]; }
};


template<class StatType>
class DistStatBase : public StatBase<StatType> { // wrapper for Stats::DistBase
  public:
    template<typename U>
    void sample(const U &v, int n = 1) { StatBase<StatType>::stat.sample(v, n); }

    void add(DistStatBase & d) { StatBase<StatType>::stat.add(d.StatBase<StatType>::stat); }
};


/*
  nice wrappers for the gem5 stats classes used throughout the rest of the code
*/

class ScalarStat : public ScalarStatBase<Stats_ramulator::Scalar> {
  public:
    using ScalarStatBase<Stats_ramulator::Scalar>::operator=;
};

class AverageStat : public ScalarStatBase<Stats_ramulator::Average> {
  public:
    using ScalarStatBase<Stats_ramulator::Average>::operator=;
};

class VectorStat : public VectorStatBase<Stats_ramulator::Vector, Stats_ramulator::Scalar> {
};

class AverageVectorStat : public VectorStatBase<Stats_ramulator::AverageVector, Stats_ramulator::Average> {
};

class DistributionStat : public DistStatBase<Stats_ramulator::Distribution> {
  protected:
    DistributionStat & self() { return *this; }

  public:
    DistributionStat & init(Stats_ramulator::Counter min, Stats_ramulator::Counter max, Stats_ramulator::Counter bkt) {
      StatBase<Stats_ramulator::Distribution>::stat.init(min, max, bkt);
      return self();
    }

};

class HistogramStat : public DistStatBase<Stats_ramulator::Histogram> {
  protected:
    HistogramStat & self() { return *this; }

  public:
    HistogramStat & init(Stats_ramulator::size_type size) {
      StatBase<Stats_ramulator::Histogram>::stat.init(size);
      return self();
    }
};

class StandardDeviationStat : public DistStatBase<Stats_ramulator::StandardDeviation> {
};

class AverageDeviationStat : public DistStatBase<Stats_ramulator::AverageDeviation> {
};

/*
  Stats TODO
  * Formula
  * VectorDistribution
  * VectorStandardDeviation
  * VectorAverageDeviation
  * Vector2d
  * SparseHistogram
*/

} /* namespace ramulator */

#endif
