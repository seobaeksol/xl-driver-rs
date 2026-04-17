#pragma once

template <typename Type, typename Functor>
class AutoCleaner {
public:
  AutoCleaner(Type value, Functor functor) : mValue{value}, mFunctor{functor} {}

  ~AutoCleaner() {
    mFunctor(mValue);
  }

private:
  Type    mValue;
  Functor mFunctor;
};