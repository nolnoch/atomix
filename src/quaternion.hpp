/**
 * quaternion.hpp
 *
 *  atomix
 * 
 *    Created on: Mar 8, 2013
 *   Last Update: Sep 9, 2024
 *  Orig. Author: Wade Burch (braernoch.dev@gmail.com)
 * 
 *  Copyright 2013,2023,2024 Wade Burch (GPLv3)
 * 
 *  This file is part of atomix.
 * 
 *  atomix is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software 
 *  Foundation, either version 3 of the License, or (at your option) any later 
 *  version.
 * 
 *  atomix is distributed in the hope that it will be useful, but WITHOUT ANY 
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS 
 *  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License along with 
 *  atomix. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef QUATERNION_HPP_
#define QUATERNION_HPP_

#include <vector>
#include <glm/glm.hpp>

enum { EULER = 2, EXPLICIT = 4, ANGLE_AXIS = 8, MATRIX = 16 };
enum { DEG = 1, RAD = 0 };

const float RAD_FAC = (float) M_PI / 180.0f;


/**
 * Quaternion class.
 */
class Quaternion {
 private:
  // Data
  float W, X, Y, Z;
  int dirty;
  std::vector<float> rotMatrix;

  // Constructor Init -- UT Servers use GCC 4.6 with no ctor delegation.
  void initQAngleAxis(float theta, float *v);
  void initQEulerAngles(float vA, float vB, float vC);
  void initQRotationMatrix(float *m);
  void initQArrayUnknown(float *m, int con, int deg_rad);

  // Internal operations
  float magnitude();
  void makeMatrix();
  void loadMatrix(float *m);
  void normalizeVector(float *v);
  Quaternion inverse();
  Quaternion pureQuaternion(float *v);
  std::vector<float> rotateVector(float *v);

 public:
  // Constructors
  Quaternion();
  Quaternion(const Quaternion &b);
  explicit Quaternion(glm::vec3 m, int deg_rad);
  explicit Quaternion(float w, float x, float y, float z);
  explicit Quaternion(float theta, std::vector<float> axis, int deg_rad);
  explicit Quaternion(float theta, glm::vec3 axis, int deg_rad);
  explicit Quaternion(float theta, float axis[3], int deg_rad);
  explicit Quaternion(float yaw, float pitch, float roll, int deg_rad);
  explicit Quaternion(float *m, int construct, int deg_rad);
  explicit Quaternion(std::vector<float> m, int con, int deg_rad);

  // Destructor
  virtual ~Quaternion();

  // Public
  std::vector<float> matrix();
  std::vector<float> rotate(std::vector<float> v);
  std::vector<float> rotate(float v[3]);
  glm::vec3 rotate(glm::vec3 v);
  void toString();
  void matrixToString();
  void zero();
  void normalize();

  // Operators
  Quaternion operator+ (const Quaternion& b);
  Quaternion operator* (const Quaternion& b);
  Quaternion& operator= (const Quaternion& b);
  Quaternion& operator+= (const Quaternion& b);
  Quaternion& operator*= (const Quaternion& b);
};

#endif /* QUATERNION_H_ */
