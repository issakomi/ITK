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
#ifndef itkConicShellInteriorExteriorSpatialFunction_hxx
#define itkConicShellInteriorExteriorSpatialFunction_hxx


namespace itk
{
template <unsigned int VDimension, typename TInput>
void
ConicShellInteriorExteriorSpatialFunction<VDimension, TInput>::SetOriginGradient(GradientType grad)
{
  m_OriginGradient = grad;

  // Normalize the origin gradient
  m_OriginGradient.GetVnlVector().normalize();
}

template <unsigned int VDimension, typename TInput>
auto
ConicShellInteriorExteriorSpatialFunction<VDimension, TInput>::Evaluate(const InputType & position) const -> OutputType
{
  using VectorType = Vector<double, VDimension>;

  // Compute the vector from the origin to the point being tested
  VectorType vecOriginToTest = position - m_Origin;

  // Compute the length of this vector
  const double vecDistance = vecOriginToTest.GetNorm();

  // Check to see if this an allowed distance
  if (!((vecDistance > m_DistanceMin) && (vecDistance < m_DistanceMax)))
  {
    return 0; // not inside the conic shell
  }
  // Normalize it
  // vecOriginToTest.GetVnlVector().normalize();
  vecOriginToTest.Normalize();

  // Create a temp vector to get around const problems
  const GradientType originGradient = m_OriginGradient;

  // Now compute the dot product
  double dotprod = originGradient * vecOriginToTest;

  if (m_Polarity == 1)
  {
    dotprod = dotprod * -1;
  }

  // Check if it meets the angle criterion
  OutputType result;
  if (dotprod > (1 - m_Epsilon))
  {
    result = 1; // it's inside the shell
  }
  else
  {
    result = 0; // it's not inside the shell
  }

  return result;
}

template <unsigned int VDimension, typename TInput>
void
ConicShellInteriorExteriorSpatialFunction<VDimension, TInput>::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "Origin: [";
  for (unsigned int i = 0; i < VDimension - 1; ++i)
  {
    os << m_Origin[i] << ", ";
  }
  os << ']' << std::endl;

  os << indent << "Gradient at origin: [";
  for (unsigned int i = 0; i < VDimension - 1; ++i)
  {
    os << m_OriginGradient[i] << ", ";
  }
  os << ']' << std::endl;

  os << indent << "DistanceMin: " << m_DistanceMin << std::endl;
  os << indent << "DistanceMax: " << m_DistanceMax << std::endl;
  os << indent << "Epsilon: " << m_Epsilon << std::endl;
  os << indent << "Polarity: " << m_Polarity << std::endl;
}
} // end namespace itk

#endif
