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
#ifndef itkDisplacementFieldTransform_h
#define itkDisplacementFieldTransform_h

#include "itkTransform.h"

#include "itkImage.h"
#include "itkVectorImage.h"
#include "itkMatrixOffsetTransformBase.h"
#include "itkImageVectorOptimizerParametersHelper.h"
#include "itkVectorInterpolateImageFunction.h"

namespace itk
{

/** \class DisplacementFieldTransform
 * \brief Provides local/dense/high-dimensionality transformation via a
 * a displacement field.
 *
 * The displacement field stores vectors of displacements, with
 * dimension \c VDimension. Transformation is performed at a given
 * point by adding the displacement at that point to the input point.
 *
 * T(x, p), x is the position, p is the local parameter at position x.
 * For a 2D example:
 *
 *  x = (x0, x1), p = (p0, p1)
 *
 * then T(x, p) is defined as:
 *
 *    T(x, p) = (T0, T1) = (x0+p0, x1+p1)
 *
 * During transformation, out-of-bounds input points are returned
 * with zero displacement.
 *
 * The displacement field is defined using an itkImage, and must be set
 * before use by the user, using \c SetDisplacementField. The image has
 * the same dimensionality as the input and output spaces, defined by
 * template parameter \c VDimension, and is an image of vectors of
 * type \c OutputVectorType, with dimensionality VDimension as well.
 *
 * An interpolator of type \c VectorInterpolateImageFunction is used with
 * the displacement field image. By default,
 * VectorLinearInterpolateImageFunction is used, and the user can override
 * using SetInterpolator.
 *
 * The displacement field data is stored using the common
 * \c OptimizerParameters type
 * in conjunction with the \c ImageVectorOptimizerParametersHelper class. This
 * allows access of the displacement field image as if it were an itkArray,
 * allowing transparent use with other classes.
 * \warning The \c SetParameters
 * method will copy the passed parameters, which can be costly since
 * displacement fields are dense and thus potentially very large.
 *
 * The \c UpdateTransformParameters method simply adds the provided
 * update array, applying the usual optional scaling factor. Derived
 * classes may provide different behavior.
 *
 * Because this is a local transform, methods that have a version that takes
 * a point must be used, such as \c TransformVector,
 * \c TransformCovariantVector, and \c TransformDiffusionTensor. Also,
 * \c ComputeJacobianWithRespectToParameters simply returns
 * an identity matrix (see method documentation),
 * and \c ComputeJacobianWithRespectToPosition should be used.
 *
 *
 * \ingroup ITKDisplacementField
 */
template <typename TParametersValueType, unsigned int VDimension>
class ITK_TEMPLATE_EXPORT DisplacementFieldTransform : public Transform<TParametersValueType, VDimension, VDimension>
{
public:
  ITK_DISALLOW_COPY_AND_MOVE(DisplacementFieldTransform);

  /** Standard class type aliases. */
  using Self = DisplacementFieldTransform;
  using Superclass = Transform<TParametersValueType, VDimension, VDimension>;
  using Pointer = SmartPointer<Self>;
  using ConstPointer = SmartPointer<const Self>;

  /** \see LightObject::GetNameOfClass() */
  itkOverrideGetNameOfClassMacro(DisplacementFieldTransform);

  /** New macro for creation of through a Smart Pointer */
  itkNewMacro(Self);

  /** InverseTransform type. */
  using typename Superclass::InverseTransformBasePointer;

  /** Scalar type. */
  using typename Superclass::ScalarType;

  /** Type of the input parameters. */
  using typename Superclass::FixedParametersType;
  using typename Superclass::FixedParametersValueType;
  using typename Superclass::ParametersType;
  using typename Superclass::ParametersValueType;

  /** Jacobian types. */
  using typename Superclass::JacobianType;
  using typename Superclass::JacobianPositionType;
  using typename Superclass::InverseJacobianPositionType;

  /** Transform category type. */
  using typename Superclass::TransformCategoryEnum;

  /** The number of parameters defining this transform. */
  using typename Superclass::NumberOfParametersType;

  /** Standard coordinate point type for this class. */
  using typename Superclass::InputPointType;
  using typename Superclass::OutputPointType;

  /** Standard vector type for this class. */
  using typename Superclass::InputVectorType;
  using typename Superclass::OutputVectorType;

  using typename Superclass::InputVectorPixelType;
  using typename Superclass::OutputVectorPixelType;

  /** Standard covariant vector type for this class */
  using typename Superclass::InputCovariantVectorType;
  using typename Superclass::OutputCovariantVectorType;

  /** Standard vnl_vector type for this class. */
  using typename Superclass::InputVnlVectorType;
  using typename Superclass::OutputVnlVectorType;

  /** Standard diffusion tensor type for this class */
  using typename Superclass::InputDiffusionTensor3DType;
  using typename Superclass::OutputDiffusionTensor3DType;

  /** Standard tensor type for this class */
  using InputTensorEigenVectorType = CovariantVector<ScalarType, InputDiffusionTensor3DType::Dimension>;
  using OutputTensorEigenVectorType = CovariantVector<ScalarType, OutputDiffusionTensor3DType::Dimension>;

  /** Derivative type */
  using typename Superclass::DerivativeType;

  /** Dimension of the domain spaces. */
  static constexpr unsigned int Dimension = VDimension;

  /** Define the displacement field type and corresponding interpolator type. */
  using DisplacementFieldType = Image<OutputVectorType, Dimension>;
  using VectorImageDisplacementFieldType = VectorImage<TParametersValueType, Dimension>;
  using DisplacementFieldPointer = typename DisplacementFieldType::Pointer;
  using DisplacementFieldConstPointer = typename DisplacementFieldType::ConstPointer;

  using InterpolatorType = VectorInterpolateImageFunction<DisplacementFieldType, ScalarType>;

  /** Standard types for the displacement Field */
  using IndexType = typename DisplacementFieldType::IndexType;
  using RegionType = typename DisplacementFieldType::RegionType;
  using SizeType = typename DisplacementFieldType::SizeType;
  using SpacingType = typename DisplacementFieldType::SpacingType;
  using DirectionType = typename DisplacementFieldType::DirectionType;
  using PointType = typename DisplacementFieldType::PointType;
  using PixelType = typename DisplacementFieldType::PixelType;

  /** Define the internal parameter helper used to access the field */
  using OptimizerParametersHelperType =
    ImageVectorOptimizerParametersHelper<ScalarType, OutputVectorType::Dimension, Dimension>;

  /** Get/Set the displacement field.
   * Set the displacement field. Create special set accessor to update
   * interpolator and assign displacement field to transform parameters
   * container. */
  /** @ITKStartGrouping */
  virtual void
  SetDisplacementField(DisplacementFieldType * field);
  virtual void
       SetDisplacementField(VectorImageDisplacementFieldType * field);
  void SetDisplacementField(std::nullptr_t) = delete;
  itkGetModifiableObjectMacro(DisplacementField, DisplacementFieldType);
  /** @ITKEndGrouping */

  /** Get/Set the inverse displacement field. This must be supplied by the user for
   * GetInverse() to work. */
  /** @ITKStartGrouping */
  virtual void
  SetInverseDisplacementField(DisplacementFieldType * inverseField);
  itkGetModifiableObjectMacro(InverseDisplacementField, DisplacementFieldType);
  /** @ITKEndGrouping */

  /** Get/Set the interpolator.
   * Create out own set accessor that assigns the displacement field. */
  /** @ITKStartGrouping */
  virtual void
  SetInterpolator(InterpolatorType * interpolator);
  itkGetModifiableObjectMacro(Interpolator, InterpolatorType);
  /** @ITKEndGrouping */

  /** Get/Set the interpolator for the inverse field.
   * Create out own set accessor that assigns the displacement field. */
  /** @ITKStartGrouping */
  virtual void
  SetInverseInterpolator(InterpolatorType * interpolator);
  itkGetModifiableObjectMacro(InverseInterpolator, InterpolatorType);
  /** @ITKEndGrouping */

  /** Get the modification time of displacement field. */
  itkGetConstReferenceMacro(DisplacementFieldSetTime, ModifiedTimeType);

  /**  Method to transform a point. Out-of-bounds points will
   * be returned with zero displacement. */
  OutputPointType
  TransformPoint(const InputPointType & inputPoint) const override;

  /**  Method to transform a vector. */
  /** @ITKStartGrouping */
  using Superclass::TransformVector;
  OutputVectorType
  TransformVector(const InputVectorType &) const override
  {
    itkExceptionMacro("TransformVector(Vector) unimplemented, use "
                      "TransformVector(Vector,Point)");
  }

  OutputVectorPixelType
  TransformVector(const InputVectorPixelType &) const override
  {
    itkExceptionMacro("TransformVector(Vector) unimplemented, use "
                      "TransformVector(Vector,Point)");
  }

  OutputVnlVectorType
  TransformVector(const InputVnlVectorType &) const override
  {
    itkExceptionMacro("TransformVector(Vector) unimplemented, use "
                      "TransformVector(Vector,Point)");
  }
  /** @ITKEndGrouping */

  /** Method to transform a tensor. */
  /** @ITKStartGrouping */
  using Superclass::TransformDiffusionTensor3D;
  OutputDiffusionTensor3DType
  TransformDiffusionTensor(const InputDiffusionTensor3DType &) const
  {
    itkExceptionMacro("TransformDiffusionTensor(Tensor) unimplemented, use "
                      "TransformDiffusionTensor(Tensor,Point)");
  }

  OutputVectorPixelType
  TransformDiffusionTensor(const InputVectorPixelType &) const
  {
    itkExceptionMacro("TransformDiffusionTensor(Tensor) unimplemented, use "
                      "TransformDiffusionTensor(Tensor,Point)");
  }
  /** @ITKEndGrouping */

  /** Method to transform a CovariantVector. */
  /** @ITKStartGrouping */
  using Superclass::TransformCovariantVector;
  OutputCovariantVectorType
  TransformCovariantVector(const InputCovariantVectorType &) const override
  {
    itkExceptionMacro("TransformCovariantVector(CovariantVector) "
                      "unimplemented, use TransformCovariantVector(CovariantVector,Point)");
  }

  OutputVectorPixelType
  TransformCovariantVector(const InputVectorPixelType &) const override
  {
    itkExceptionMacro("TransformCovariantVector(CovariantVector) "
                      "unimplemented, use TransformCovariantVector(CovariantVector,Point)");
  }
  /** @ITKEndGrouping */

  /** Set the transformation parameters. This sets the displacement
   * field image directly. */
  void
  SetParameters(const ParametersType & params) override
  {
    if (&(this->m_Parameters) != &params)
    {
      if (params.Size() != this->m_Parameters.Size())
      {
        itkExceptionMacro("Input parameters size (" << params.Size() << ") does not match internal size ("
                                                    << this->m_Parameters.Size() << ").");
      }
      // Copy into existing object
      this->m_Parameters = params;
      this->Modified();
    }
  }

  /**
   * This method sets the fixed parameters of the transform.
   * For a displacement field transform, the fixed parameters are the
   * following: field size, field origin, field spacing, and field direction.
   *
   * Note: If a displacement field already exists, this function
   * creates a new one with zero displacement (identity transform). If
   * an inverse displacement field exists, a new one is also created.
   */
  void
  SetFixedParameters(const FixedParametersType &) override;

  /**
   * Compute the jacobian with respect to the parameters at a point.
   * Simply returns identity matrix, sized [VDimension, VDimension].
   *
   * T(x, p), x is the position, p is the local parameter at position x.
   * Take a 2D example, x = (x0, x1), p = (p0, p1) and T(x, p) is defined as:
   *
   *    T(x, p) = (T0, T1) = (x0+p0, x1+p1)
   *
   * Each local deformation is defined as a translation transform.
   * So the Jacobian w.r.t parameters are
   *
   * dT/dp =
   *    [ dT0/dp0, dT0/dp1;
   *      dT1/dp0, dT1/dp1 ];
   *
   *    = [1, 0;
   *       0, 1];
   *
   * TODO: format the above for doxygen formula.
   */
  void
  ComputeJacobianWithRespectToParameters(const InputPointType &, JacobianType & j) const override
  {
    j = this->m_IdentityJacobian;
  }

  /**
   * Compute the jacobian with respect to the parameters at an index.
   * Simply returns identity matrix, sized [VDimension, VDimension].
   * See \c ComputeJacobianWithRespectToParameters( InputPointType, ... )
   * for rationale.
   */
  virtual void
  ComputeJacobianWithRespectToParameters(const IndexType &, JacobianType & j) const
  {
    j = this->m_IdentityJacobian;
  }

  /**
   * Compute the jacobian with respect to the position, by point.
   * \c j will be resized as needed.
   */
  void
  ComputeJacobianWithRespectToPosition(const InputPointType & point, JacobianPositionType & jacobian) const override;
  using Superclass::ComputeJacobianWithRespectToPosition;

  /**
   * Compute the jacobian with respect to the position, by point.
   * \c j will be resized as needed.
   */
  void
  ComputeInverseJacobianWithRespectToPosition(const InputPointType &        point,
                                              InverseJacobianPositionType & jacobian) const override;
  using Superclass::ComputeInverseJacobianWithRespectToPosition;

  /**
   * Compute the jacobian with respect to the position, by index.
   * \c j will be resized as needed.
   */
  virtual void
  ComputeJacobianWithRespectToPosition(const IndexType & index, JacobianPositionType & jacobian) const;

  /**
   * Compute the inverse jacobian of the forward displacement field with
   * respect to the position, by point. Note that this is different than
   * the jacobian of the inverse displacement field. This takes advantage
   * of the ability to compute the inverse jacobian of a displacement field
   * by simply reversing the sign of the forward jacobian.
   * However, a more accurate method for computing the inverse
   * jacobian is to take the inverse of the jacobian matrix. This
   * method is more computationally expensive and may be used by
   * setting \c useSVD to true
   */
  virtual void
  GetInverseJacobianOfForwardFieldWithRespectToPosition(const InputPointType & point,
                                                        JacobianPositionType & jacobian,
                                                        bool                   useSVD = false) const;

  /**
   * Compute the inverse jacobian of the forward displacement field with
   * respect to the position, by index.Note that this is different than
   * the jacobian of the inverse displacement field. This takes advantage
   * of the ability to compute the inverse jacobian of a displacement field
   * by simply reversing the sign of the forward jacobian.
   * However, a more accurate method for computing the inverse
   * jacobian is to take the inverse of the jacobian matrix. This
   * method is more computationally expensive and may be used by
   * setting \c useSVD to true
   */
  virtual void
  GetInverseJacobianOfForwardFieldWithRespectToPosition(const IndexType &      index,
                                                        JacobianPositionType & jacobian,
                                                        bool                   useSVD = false) const;

  void
  UpdateTransformParameters(const DerivativeType & update, ScalarType factor = 1.0) override;

  /** Return an inverse of this transform.
   * Note that the inverse displacement field must be set by the user. */
  bool
  GetInverse(Self * inverse) const;

  /** Return an inverse of this transform.
   * Note that the inverse displacement field must be set by the user. */
  InverseTransformBasePointer
  GetInverseTransform() const override;

  virtual void
  SetIdentity();

  /** This transform is not linear. */
  TransformCategoryEnum
  GetTransformCategory() const override
  {
    return Self::TransformCategoryEnum::DisplacementField;
  }

  NumberOfParametersType
  GetNumberOfLocalParameters() const override
  {
    return Dimension;
  }

  /** Set/Get the coordinate tolerance.
   *  This tolerance is used when comparing the space defined
   *  by deformation fields and its inverse to ensure they occupy the
   *  same physical space.
   *
   * \sa ImageToImageFilterCommon::SetGlobalDefaultCoordinateTolerance
   */
  /** @ITKStartGrouping */
  itkSetMacro(CoordinateTolerance, double);
  itkGetConstMacro(CoordinateTolerance, double);
  /** @ITKEndGrouping */

  /** Set/Get the direction tolerance.
   *  This tolerance is used to when comparing the orientation of the
   *  deformation fields and its inverse to ensure they occupy the
   *  same physical space.
   *
   * \sa ImageToImageFilterCommon::SetGlobalDefaultDirectionTolerance
   */
  /** @ITKStartGrouping */
  itkSetMacro(DirectionTolerance, double);
  itkGetConstMacro(DirectionTolerance, double);
  /** @ITKEndGrouping */

protected:
  DisplacementFieldTransform();
  ~DisplacementFieldTransform() override = default;
  void
  PrintSelf(std::ostream & os, Indent indent) const override;

  /** The displacement field and its inverse (if it exists). */
  typename DisplacementFieldType::Pointer m_DisplacementField{};
  typename DisplacementFieldType::Pointer m_InverseDisplacementField{};

  /** The interpolator. */
  typename InterpolatorType::Pointer m_Interpolator{};
  typename InterpolatorType::Pointer m_InverseInterpolator{};

  /** Track when the displacement field was last set/assigned, as
   * distinct from when it may have had its contents modified. */
  ModifiedTimeType m_DisplacementFieldSetTime{ 0 };

  /** Create an identity jacobian for use in
   * ComputeJacobianWithRespectToParameters. */
  JacobianType m_IdentityJacobian{};

private:
  /** Internal method for calculating either forward or inverse jacobian,
   * depending on state of \c doInverseJacobian. Used by
   * public methods \c ComputeJacobianWithRespectToPosition and
   * \c GetInverseJacobianOfForwardFieldWithRespectToPosition to
   * perform actual work.
   * \c doInverseJacobian indicates that the inverse jacobian
   * should be returned
   */
  virtual void
  ComputeJacobianWithRespectToPositionInternal(const IndexType &      index,
                                               JacobianPositionType & jacobian,
                                               bool                   doInverseJacobian) const;

  /**
   * Internal method to check that the inverse and forward displacement fields have the
   * same fixed parameters.
   */
  virtual void
  VerifyFixedParametersInformation();

  /**
   * Convenience method which reads the information from the current
   * displacement field into m_FixedParameters.
   */
  virtual void
  SetFixedParametersFromDisplacementField() const;

  double m_CoordinateTolerance{};
  double m_DirectionTolerance{};
};

} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#  include "itkDisplacementFieldTransform.hxx"
#endif

#endif // itkDisplacementFieldTransform_h
