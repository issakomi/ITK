/*=========================================================================
 *
 *  Copyright NumFOCUS
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         https://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#include "itkObjectStore.h"
#include <iostream>
#include <set>

struct TestObject
{
  float vector[3];
  int   counter;
};

int
itkObjectStoreTest(int, char *[])
{

  const itk::ObjectStore<TestObject>::Pointer store = itk::ObjectStore<TestObject>::New();

  std::list<TestObject *> borrowed_list;
  store->SetGrowthStrategyToExponential();
  store->Reserve(40000);

  // for increased code coverage
  store->SetLinearGrowthSize(store->GetLinearGrowthSize());
  store->SetGrowthStrategy(store->GetGrowthStrategy());


  for (unsigned int j = 0; j < 2; ++j)
  {
    std::cout << "_______________________" << std::endl;
    store->Print(std::cout);


    // Borrow some items
    for (unsigned int i = 0; i < 30000; ++i)
    {
      borrowed_list.push_back(store->Borrow());
    }
    store->Print(std::cout);

    // Force allocation of a more memory
    for (unsigned int i = 0; i < 1000000; ++i)
    {
      borrowed_list.push_back(store->Borrow());
    }
    store->Print(std::cout);

    // Return all items
    auto sz = static_cast<unsigned int>(borrowed_list.size());
    for (unsigned int i = 0; i < sz; ++i)
    {
      store->Return(borrowed_list.back());
      borrowed_list.pop_back();
    }
    store->Print(std::cout);

    store->Clear();
    store->Squeeze();
  }

  // Test streaming enumeration for ObjectStoreEnums::GrowthStrategy elements
  const std::set<itk::ObjectStoreEnums::GrowthStrategy> allGrowthStrategy{
    itk::ObjectStoreEnums::GrowthStrategy::LINEAR_GROWTH, itk::ObjectStoreEnums::GrowthStrategy::EXPONENTIAL_GROWTH
  };
  for (const auto & ee : allGrowthStrategy)
  {
    std::cout << "STREAMED ENUM VALUE ObjectStoreEnums::GrowthStrategy: " << ee << std::endl;
  }
  return EXIT_SUCCESS;
}
