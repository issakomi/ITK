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
#ifndef itkVectorContainerToListSampleAdaptor_h
#define itkVectorContainerToListSampleAdaptor_h

#include <typeinfo>

#include "itkListSample.h"
#include "itkSmartPointer.h"
#include "itkVectorContainer.h"

namespace itk::Statistics
{
/** \class VectorContainerToListSampleAdaptor
 *  \brief This class provides ListSample interface to ITK VectorContainer
 *
 * After calling SetVectorContainer(VectorContainer*) method to plug-in
 * the VectorContainer object, users can use Sample interfaces to access
 * VectorContainer data. This adaptor assumes that the VectorContainer is
 * actual storage for measurement vectors. In other words, VectorContainer's
 * element dimension equals to the measurement vectors size.
 *
 * \sa Sample, ListSample, VectorContainer
 *
 * \ingroup ITKStatistics
 */

template <typename TVectorContainer>
class ITK_TEMPLATE_EXPORT VectorContainerToListSampleAdaptor : public ListSample<typename TVectorContainer::Element>
{
public:
  ITK_DISALLOW_COPY_AND_MOVE(VectorContainerToListSampleAdaptor);

  /** Standard class type aliases */
  using Self = VectorContainerToListSampleAdaptor;
  using Superclass = ListSample<typename TVectorContainer::Element>;
  using Pointer = SmartPointer<Self>;
  using ConstPointer = SmartPointer<const Self>;

  /** \see LightObject::GetNameOfClass() */
  itkOverrideGetNameOfClassMacro(VectorContainerToListSampleAdaptor);

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** The number of components in a measurement vector. */
  static constexpr unsigned int MeasurementVectorSize = TVectorContainer::Element::Dimension;

  /** VectorContainer type alias. */
  using VectorContainerType = TVectorContainer;
  using VectorContainerPointer = typename TVectorContainer::Pointer;
  using VectorContainerConstPointer = typename TVectorContainer::ConstPointer;
  using VectorContainerIterator = typename TVectorContainer::Iterator;
  using VectorContainerConstIterator = typename TVectorContainer::ConstIterator;

  /** Superclass type alias for Measurement vector, measurement,
   * Instance Identifier, frequency, size, size element value */
  using typename Superclass::MeasurementType;
  using typename Superclass::MeasurementVectorType;
  using typename Superclass::AbsoluteFrequencyType;
  using typename Superclass::TotalAbsoluteFrequencyType;
  using typename Superclass::MeasurementVectorSizeType;
  using typename Superclass::InstanceIdentifier;

  using ValueType = MeasurementVectorType;

  /** Get/Set method for the point set container which will be actually used
   * for storing measurement vectors. */
  /** @ITKStartGrouping */
  itkSetObjectMacro(VectorContainer, VectorContainerType);
  itkGetConstObjectMacro(VectorContainer, VectorContainerType);
  /** @ITKEndGrouping */
  /** Returns the number of measurement vectors in this container. */
  InstanceIdentifier
  Size() const override;

  /** Returns the measurement vector that is specified by the instance
   * identifier argument. */
  const MeasurementVectorType & GetMeasurementVector(InstanceIdentifier) const override;

  /** Returns 1 as other subclasses of ListSampleBase does. */
  AbsoluteFrequencyType GetFrequency(InstanceIdentifier) const override;

  /** Returns the size of this container. */
  TotalAbsoluteFrequencyType
  GetTotalFrequency() const override;

  /** \class ConstIterator
   * \ingroup ITKStatistics
   */
  class ConstIterator
  {
    friend class VectorContainerToListSampleAdaptor;

  public:
    ConstIterator(const VectorContainerToListSampleAdaptor * adaptor) { *this = adaptor->Begin(); }

    ConstIterator(const ConstIterator & iter)
      : m_Iter(iter.m_Iter)
      , m_InstanceIdentifier(iter.m_InstanceIdentifier)
    {}

    ConstIterator() = delete;

    ConstIterator &
    operator=(const ConstIterator & iter) = default;

    [[nodiscard]] AbsoluteFrequencyType
    GetFrequency() const
    {
      return 1;
    }

    [[nodiscard]] const MeasurementVectorType &
    GetMeasurementVector() const
    {
      return (const MeasurementVectorType &)m_Iter.Value();
    }

    [[nodiscard]] InstanceIdentifier
    GetInstanceIdentifier() const
    {
      return this->m_InstanceIdentifier;
    }

    ConstIterator &
    operator++()
    {
      ++m_Iter;
      ++m_InstanceIdentifier;
      return *this;
    }

    bool
    operator==(const ConstIterator & it) const
    {
      return (this->m_Iter == it.m_Iter);
    }

    ITK_UNEQUAL_OPERATOR_MEMBER_FUNCTION(ConstIterator);

  protected:
    // This method should only be available to the ListSample class
    ConstIterator(VectorContainerConstIterator iter, InstanceIdentifier iid)
      : m_Iter(iter)
      , m_InstanceIdentifier(iid)
    {}

  private:
    VectorContainerConstIterator m_Iter;
    InstanceIdentifier           m_InstanceIdentifier;
  };

  /** \class Iterator
   * \ingroup ITKStatistics
   */
  class Iterator : public ConstIterator
  {
    friend class VectorContainerToListSampleAdaptor;

  public:
    Iterator() = delete;
    Iterator(const Self * adaptor) = delete;
    Iterator(VectorContainerConstIterator iter, InstanceIdentifier iid) = delete;
    Iterator(const ConstIterator & it) = delete;
    ConstIterator &
    operator=(const ConstIterator & it) = delete;

    Iterator(Self * adaptor)
      : ConstIterator(adaptor)
    {}

    Iterator(const Iterator & iter)
      : ConstIterator(iter)
    {}

    Iterator &
    operator=(const Iterator & iter)
    {
      this->ConstIterator::operator=(iter);
      return *this;
    }

  protected:
    Iterator(VectorContainerIterator iter, InstanceIdentifier iid)
      : ConstIterator(iter, iid)
    {}
  };

  /** Returns an iterator that points to the beginning of the container. */
  Iterator
  Begin()
  {
    VectorContainerPointer nonConstVectorDataContainer =
      const_cast<VectorContainerType *>(this->m_VectorContainer.GetPointer());
    Iterator iter(nonConstVectorDataContainer->Begin(), 0);

    return iter;
  }

  /** Returns an iterator that points to the end of the container. */
  Iterator
  End()
  {
    VectorContainerPointer nonConstVectorDataContainer =
      const_cast<VectorContainerType *>(this->m_VectorContainer.GetPointer());

    Iterator iter(nonConstVectorDataContainer->End(), this->m_VectorContainer->Size());

    return iter;
  }

  /** Returns an iterator that points to the beginning of the container. */
  ConstIterator
  Begin() const
  {
    const ConstIterator iter(this->m_VectorContainer->Begin(), 0);

    return iter;
  }

  /** Returns an iterator that points to the end of the container. */
  ConstIterator
  End() const
  {
    const ConstIterator iter(this->m_VectorContainer->End(), this->m_VectorContainer->Size());
    return iter;
  }

protected:
  VectorContainerToListSampleAdaptor();

  ~VectorContainerToListSampleAdaptor() override = default;
  void
  PrintSelf(std::ostream & os, Indent indent) const override;

private:
  VectorContainerConstPointer m_VectorContainer{};
}; // end of class VectorContainerToListSampleAdaptor
} // namespace itk::Statistics

#ifndef ITK_MANUAL_INSTANTIATION
#  include "itkVectorContainerToListSampleAdaptor.hxx"
#endif

#endif
