/**
 * quaternion.cpp
 *
 *  atomix
 * 
 *    Created on: Mar 8, 2013
 *   Last Update: Dec 29, 2024
 *  Orig. Author: Wade Burch (dev@nolnoch.com)
 * 
 *  This Quaternion class was designed for use in CS354 Computer Graphics.
 *  This may not be the most efficient way to reinvent the wheel, but I
 *  wanted to understand each and every manipulation as deeply as I could,
 *  and that required writing it in my own style without any use of copy
 *  and paste. Some notes about the implementation:
 *
 *  [Quaternion Representation]
 *
 *  One can visualize a quaternion's components in the formats:
 *
 *        Q = [ X, Y, Z, W ]        or         Q = [ W | X, Y, Z ]
 *
 *  where the former is more similar to a vector in homogeneous coordinates
 *  and the latter is more in line with angle-axis representation. Because
 *  it helps me to think of a quaternion as a physical representation of a
 *  rotation, I've chosen the latter in this code.
 *
 *
 *  [Return Types]
 *
 *  There was an interesting challenge with return types. For instance, the
 *  matrix representation of the quaternion is expected in OpenGL as a float
 *  array. To return an array, I'd have to allocate it to the heap (No! I
 *  don't trust programmers to free it later), pass a pointer to a permanent
 *  local copy (No...it solves the memory leak but opens our class copy to
 *  being tampered with), or return some kind of object that could cast to
 *  a float array.  Luckily, OpenGL technically expects,
 *
 *      "a pointer to 16 consecutive values, which are used as the
 *      elements of a 4x4 column-major matrix,"
 *
 *  which an STL vector certainly satisfies. Thus, all array returns are
 *  passed as copies of an STL vector, leaving memory clean and our
 *  variables safe.
 *
 *
 *  [Vector/vertex Rotation]
 *
 *  It would make sense to overload the operator* to handle the application
 *  of a quaternion to a vector (v' = q*v*q^), but I've chosen to make it
 *  a public function to reinforce the concept of the quaternion being
 *  the mathematical embodiment of a rotation which acts upon vector. This
 *  will work as well for 'concatenated' quaternions. Example:
 *
 *      v2 = (qA * qB * qC).rotate(v1);         // v' = Ra.Rb.Rc.v
 *
 *
 *  [Constructor Overload]
 *
 *  Pun intended. I'm trying to cover all bases for the instantiation of a
 *  quaternion from different representations of rotations and even different
 *  methods of passing array values. When it becomes safe to depend on
 *  constructor delegation (GCC 4.7), I'll refactor for a cleaner class.
 *  I also considered the class template format, but that prohibits the split
 *  of code between a .cpp and a .h file. I'd rather be well-documented and
 *  organized for this learning experience.
 *
 *
 *  [Miscellaneous]
 *
 *  The liberal use of the keyword "this" is a personal style choice for
 *  enforced correctness and clarity.
 *
 *
 *  [Resources]
 *
 *  My four favourite resources for understanding quaternions (in addition
 *  to Wikipedia) are listed below:
 *
 *  http://www.youtube.com/watch?v=LB6U849kwXc
 *  http://www.euclideanspace.com/maths/geometry/rotations/conversions/
 *  http://www.cs.princeton.edu/~gewang/projects/darth/stuff/quat_faq.html
 *  http://content.gpwiki.org/index.php/
 *                 OpenGL:Tutorials:Using_Quaternions_to_represent_rotation
 *
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

#include "./quaternion.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <assert.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>

const float N_TOLERANCE = 1.0001f;          // Normalization tolerance
const float T_TOLERANCE = 0.0001f;          // Trace tolerance


/***************************
 * Constructors
 */



/**
 * Default constructor.
 * Initializes to the zero quaternion.
 */
Quaternion::Quaternion()
: W(1), X(0), Y(0), Z(0), dirty(1) {
  this->rotMatrix.resize(16);
}

/**
 * Copy constructor.
 */
Quaternion::Quaternion(const Quaternion &b)
: W(b.W), X(b.X), Y(b.Y), Z(b.Z), dirty(1) {
  this->rotMatrix.clear();
  //TODO implement actual copy.
}

/************************************************************************
 * The following constructor stubs are necessary because the UT servers
 * use GCC 4.6, which supports most of the features of C++11 EXCEPT
 * constructor delegation. Reference the called init functions for
 * descriptions of their implementations.
 */

/**
 * Explicit construction of a quaternion.
 * Four terms listed individually.
 * @param w - the W, or rotation, value
 * @param x - the X component of the arbitrary axis
 * @param y - the Y component of the arbitrary axis
 * @param z - the Z component of the arbitrary axis
 */
Quaternion::Quaternion(float w, float x, float y, float z)
: W(w), X(x), Y(y), Z(z), dirty(1) {
  this->rotMatrix.resize(16);

  // Only normalize if not a pure quaternion.
  if (W)
    this->normalize();
}

/**
 * Create quaternion from Euler angles - yaw (Z), pitch (Y), and roll (X).
 * Three terms listed individually.
 * @param yaw - rotation about the Z axis
 * @param pitch - rotation about the Y axis
 * @param roll - rotation about the X axis
 * @param deg_rad - constant flag specifying degrees [DEG] or radians [RAD]
 */
Quaternion::Quaternion(float yaw, float pitch, float roll, int deg_rad)
: W(0), X(0), Y(0), Z(0), dirty(1) {
  if (deg_rad) {
    yaw *= (RAD_FAC);
    pitch *= (RAD_FAC);
    roll *= (RAD_FAC);
  }
  this->initQEulerAngles(yaw, pitch, roll);
}

/**
 * Create quaternion from Euler angles - yaw (Z), pitch (Y), and roll (X).
 * Three terms given by a glm::vec3 array.
 * @param v3Angles - glm::vec3 of Z, Y, and X rotation values
 * @param deg_rad - constant flag specifying degrees [DEG] or radians [RAD]
 */
Quaternion::Quaternion(glm::vec3 v3Angles, int deg_rad)
: W(0), X(0), Y(0), Z(0), dirty(1) {
  if (deg_rad) {
    v3Angles[0] *= (RAD_FAC);
    v3Angles[1] *= (RAD_FAC);
    v3Angles[2] *= (RAD_FAC);
  }
  this->initQEulerAngles(v3Angles[0], v3Angles[1], v3Angles[2]);
}

/**
 * Create quaternion from an angle and axis of rotation.
 * Axis given by a std::vector<float> array of 3 terms.
 * @param theta - angle of rotation
 * @param axis - axis of rotation
 * @param deg_rad - constant flag specifying degrees [DEG] or radians [RAD]
 */
Quaternion::Quaternion(float theta, std::vector<float> axis, int deg_rad)
: W(0), X(0), Y(0), Z(0), dirty(1) {
  if (deg_rad) {
    theta *= (RAD_FAC);
  }
  this->initQAngleAxis(theta, &axis[0]);
}

/**
 * Create quaternion from an angle and axis of rotation.
 * Axis given by a glm::vec3 float array of 3 terms.
 * @param theta - angle of rotation
 * @param axis - axis of rotation
 * @param deg_rad - constant flag specifying degrees [DEG] or radians [RAD]
 */
Quaternion::Quaternion(float theta, glm::vec3 axis, int deg_rad)
: W(0), X(0), Y(0), Z(0), dirty(1) {
  if (deg_rad) {
    theta *= (RAD_FAC);
  }
  this->initQAngleAxis(theta, glm::value_ptr(axis));
}

/**
 * Create quaternion from an angle and axis of rotation.
 * Axis given by a primitive float array of 3 terms.
 * @param theta - angle of rotation
 * @param axis - axis of rotation
 * @param deg_rad - constant flag specifying degrees [DEG] or radians [RAD]
 */
Quaternion::Quaternion(float theta, float axis[3], int deg_rad)
: W(0), X(0), Y(0), Z(0), dirty(1) {
  if (deg_rad) {
    theta *= (RAD_FAC);
  }
  this->initQAngleAxis(theta, axis);
}

/**
 * Takes an array in the form of a float[].
 * Depending on the flag given, create from:
 * EULER      - Euler Angles [Z, Y, X]
 * EXPLICIT   - Explicit [W, X, Y, Z]
 * ANGLE_AXIS - Angle-Axis [Theta, X, Y, Z]
 * MATRIX     - Rotation Matrix std::vector<float>[16]
 * @param m - float[] of size 3, 4, or 16
 * @param construct - constant flag to determine how the values are used:
 *                       EULER, EXPLICIT, ANGLE_AXIS, or MATRIX
 * @param deg_rad - constant flag specifying degrees [DEG] or radians [RAD]
 */
Quaternion::Quaternion(float *m, int construct, int deg_rad)
: W(0), X(0), Y(0), Z(0), dirty(1) {
  this->initQArrayUnknown(m, construct, deg_rad);
}

/**
 * Takes an array in the form of a std::vector<float>.
 * Depending on the flag given, create from:
 * EULER      - Euler Angles [Z, Y, X]
 * EXPLICIT   - Explicit [W, X, Y, Z]
 * ANGLE_AXIS - Angle-Axis [Theta, X, Y, Z]
 * MATRIX     - Rotation Matrix std::vector<float>[16]
 * @param m - std::vector<float> of size 3, 4, or 16
 * @param construct - constant flag to determine how the values are used:
 *                       EULER, EXPLICIT, ANGLE_AXIS, or MATRIX
 * @param deg_rad - constant flag specifying degrees [DEG] or radians [RAD]
 */
Quaternion::Quaternion(std::vector<float> m, int construct, int deg_rad)
: W(0), X(0), Y(0), Z(0), dirty(1) {
  this->initQArrayUnknown(&m[0], construct, deg_rad);
}

/**
 * Destruct this quaternion.
 */
Quaternion::~Quaternion() {
  // Fill in if necessary.
}




/***************************
 * Private
 */



/**
 * Initialization for creating a quaternion from angle-axis. This is the
 * simplest conversion and generally the first example of a quaternion.
 * @param theta - angle of rotation
 * @param axis - axis of rotation
 */
void Quaternion::initQAngleAxis(float theta, float *axis) {
  float a[] = {axis[0], axis[1], axis[2]};
  float sinTheta = sin(theta / 2.0f);

  normalizeVector(a);

  this->W = cos(theta / 2.0f);
  this->X = a[0] * sinTheta;
  this->Y = a[1] * sinTheta;
  this->Z = a[2] * sinTheta;

  this->dirty = 1;
  this->rotMatrix.resize(16);
  this->normalize();
}

/**
 * Create quaternion from yaw, pitch, and roll. The simple but inefficient
 * solution is to create a quaternion from each angle and multiply. We can
 * save memory and cycles by directly converting the multiplied Euler angle
 * transform matrix (in order Z-->Y-->X) to quaternion representation.
 * @param yaw - Z axis rotation
 * @param pitch - Y axis rotation
 * @param roll - X axis rotation
 */
void Quaternion::initQEulerAngles(float yaw, float pitch, float roll) {
  float sinZ = sin(yaw / 2.0f);
  float sinY = sin(pitch / 2.0f);
  float sinX = sin(roll / 2.0f);
  float cosZ = cos(yaw / 2.0f);
  float cosY = cos(pitch / 2.0f);
  float cosX = cos(roll / 2.0f);

  // Started with Wikipedia, but had to swap all X & Z values (empirical).
  this->W = (cosX * cosY * cosZ) + (sinX * sinY * sinZ);
  this->X = (sinX * cosY * cosZ) - (cosX * sinY * sinZ);
  this->Y = (cosX * sinY * cosZ) + (sinX * cosY * sinZ);
  this->Z = (cosX * cosY * sinZ) - (sinX * sinY * cosZ);

  this->dirty = 1;
  this->rotMatrix.resize(16);
  this->normalize();
}

/**
 * Create quaternion from a rotation matrix. This is fairly difficult.
 * Essentially, if the trace (T) is greater than 0, you can immediately
 * derive the quaternion based on the fact that T = 2w^2 + 2w^2 - 1,
 * thus sqrt(T + 1) / 2 = w. The conversion from quaternion to matrix
 * (also in this class) shows *very* similar equations that allow the
 * x, y, and z components to be derived in the same way.
 *
 * It gets still more complicated if T is equal to or less than 0.
 * In this case, choose the greatest of the diagonal entries in the
 * rotation matrix, and re-calculate the "trace" before carrying out
 * the above-mentioned derivation of the quaternion components.
 *
 * @param m - a linear 16-value array representing the 4x4 affine rotation
 */
void Quaternion::initQRotationMatrix(float *m) {
  float trace = m[0] + m[5] + m[10] + 1.0f;
  float s;

  if (trace > T_TOLERANCE) {
    s = sqrt(trace) * 2.0f;
    this->W = s * 0.25f;
    this->X = (m[6] - m[9]) / s;
    this->Y = (m[8] - m[2]) / s;
    this->Z = (m[1] - m[4]) / s;
  } else {
    if (m[0] > m[5] && m[0] > m[10]) {
      s = sqrt(1.0f + m[0] - m[5] - m[10]) * 2.0f;
      this->W = (m[6] - m[9]) / s;
      this->X = s * 0.25f;
      this->Y = (m[1] + m[4]) / s;
      this->Z = (m[8] + m[2]) / s;
    } else if (m[5] > m[10]) {
      s  = sqrt(1.0f + m[5] - m[0] - m[10]) * 2.0f;
      this->W = (m[8] - m[2]) / s;
      this->X = (m[1] + m[4]) / s;
      this->Y = s * 0.25f;
      this->Z = (m[6] + m[9]) / s;
    } else {
      s  = sqrt(1.0f + m[10] - m[0] - m[5]) * 2.0f;
      this->W = (m[1] - m[4]) / s;
      this->X = (m[8] + m[2]) / s;
      this->Y = (m[6] + m[9]) / s;
      this->Z = s * 0.25f;
    }
  }

  this->rotMatrix.resize(16);
  this->loadMatrix(m);
  this->normalize();

  // This is logically unnecessary after loadMatrix, but required in a ctor.
  this->dirty = 0;
}

/**
 * Handles the quaternion construction from the array of unknown size based
 * on the given flag constants.
 * Depending on the flag given, create from:
 * EULER      - Euler Angles [Z, Y, X]
 * EXPLICIT   - Explicit [W, X, Y, Z]
 * ANGLE_AXIS - Angle-Axis [Theta, X, Y, Z]
 * MATRIX     - Rotation Matrix std::vector<float>[16]
 * @param m - std::vector<float> of size 3, 4, or 16
 * @param construction - constant flag to determine how the values are used:
 *                       EULER, EXPLICIT, ANGLE_AXIS, or MATRIX
 * @param deg_rad - constant flag specifying degrees [DEG] or radians [RAD]
 */
void Quaternion::initQArrayUnknown(float *m, int construction, int deg_rad) {
  switch (construction) {
    case EULER: {
      if (deg_rad) {
        m[0] *= (RAD_FAC);
        m[1] *= (RAD_FAC);
        m[2] *= (RAD_FAC);
      }
      this->initQEulerAngles(m[0], m[1], m[2]);
      break;
    }
    case EXPLICIT: {
      W = m[0];
      X = m[1];
      Y = m[2];
      Z = m[3];
      this->rotMatrix.resize(16);
      this->normalize();
      break;
    }
    case ANGLE_AXIS: {
      if (deg_rad) {
        m[0] *= (RAD_FAC);
      }
      initQAngleAxis(m[0], &m[1]);
      break;
    }
    case MATRIX: {
      this->initQRotationMatrix(m);
      break;
    }
    default: {
      this->W = 1;
      break;
    }
  }
}

/**
 * The magnitude of a quaternion is similar to that of a standard vector.
 * That is, the root of the sum of the squares.
 * @return the magnitude of this quaternion
 */
float Quaternion::magnitude() {
  float mag = (this->W * this->W) + (this->X * this->X) +
      (this->Y * this->Y) + (this->Z * this->Z);

  return sqrt(mag);
}

/**
 * Normalize this quaternion. Only proceeds if quaternion is not already
 * normalized. Given tolerance to compensate for float inaccuracies.
 */
void Quaternion::normalize() {
  float mag = this->magnitude();

  // May avoid the sqrt() call to save clock cycles, but calling magnitude
  // instead preserves modularity.
  if (mag > N_TOLERANCE) {
    this->W = W / mag;
    this->X = X / mag;
    this->Y = Y / mag;
    this->Z = Z / mag;
  }
}

/**
 * Compute the rotation matrix representation of this quaternion instance.
 * This should only be called when the matrix is publicly requested AND an
 * up-to-date representation does not already exist.
 */
void Quaternion::makeMatrix() {
  // I'm including the (x2) in the local variable declarations to minimize
  // the loss of "inconsequential" values in the later addition. (Valid?)
  float xw = this->X * this->W * 2.0f;
  float xx = this->X * this->X * 2.0f;
  float xy = this->X * this->Y * 2.0f;
  float xz = this->X * this->Z * 2.0f;
  float yw = this->Y * this->W * 2.0f;
  float yy = this->Y * this->Y * 2.0f;
  float yz = this->Y * this->Z * 2.0f;
  float zw = this->Z * this->W * 2.0f;
  float zz = this->Z * this->Z * 2.0f;

  // This is transposed from the Princeton document's solution, presumably
  // because of column-major storage. Change based on my testing.
  float m[16] = {
      1.0f - (yy + zz),           (xy + zw),           (xz - yw),          0,
             (xy - zw),    1.0f - (xx + zz),           (yz + xw),          0,
             (xz + yw),           (yz - xw),    1.0f - (xx + yy),          0,
                     0,                   0,                   0,          1
  };

  // Load the matrix into the class variable.
  this->loadMatrix(m);
}

/**
 * Load a given rotation matrix into the instance matrix. Can then be used
 * to derive a new quaternion.
 * @param m - a linear 16-value array representing the 4x4 affine rotation
 */
void Quaternion::loadMatrix(float *m) {
  assert(this->rotMatrix.capacity() == 16);

  for (int i = 0; i < 16; i++) {
    this->rotMatrix[i] = m[i];
  }

  // Flag matrix as up-to-date.
  this->dirty = 0;
}

/**
 * Normalizes the vector. Used before making a quaternion from angle-axis
 * parameters so that the result may be pre-normalized.
 * @param v - the vector to be normalized
 */
void Quaternion::normalizeVector(float *v) {
  float mag = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

  v[0] /= mag;
  v[1] /= mag;
  v[2] /= mag;
}

/**
 * Create quaternion inverse or conjugate from current.
 * @return a copy of the inverse quaternion
 */
Quaternion Quaternion::inverse() {
  return Quaternion(this->W, -this->X, -this->Y, -this->Z);
}

/**
 * Create pure quaternion from a vector or vertex.
 * Essentially puts the vector into homogeneous coordinate form.
 * @param v - the standard vector to be quaternized
 * @return a copy of the pure quaternion
 */
Quaternion Quaternion::pureQuaternion(float *v) {
  return Quaternion(0.0f, v[0], v[1], v[2]);
}

/**
 * Applies a quaternion rotation to a single vector or vertex represented by
 * a 3-value array. Makes a pure quaternion (w) from the vector and applies
 * the formula: (v' = q * w * q^) where q^ is the inverse of quaternion q.
 * @param v - the vector or vertex to be rotated
 * @return a copy of the rotated vector or vertex
 */
std::vector<float> Quaternion::rotateVector(float *v) {
  Quaternion qPure = pureQuaternion(v);
  Quaternion qInv = this->inverse();

  Quaternion qResult = (*this * (qPure * qInv));

  std::vector<float> vRotated;
  vRotated.push_back(qResult.X);
  vRotated.push_back(qResult.Y);
  vRotated.push_back(qResult.Z);

  return vRotated;
}




/***************************
 * Public
 */



/**
 * Provides a copy of the current rotation matrix representation of this
 * quaternion. If the matrix representation has not been computed or has
 * been changed recently, perform that calculation first.
 * @return a copy of the instance's rotation matrix representation
 */
std::vector<float> Quaternion::matrix() {
  if (this->dirty)
    this->makeMatrix();

  return this->rotMatrix;
}

/**
 * Perform a quaternion rotation on the passed vector object. Because it
 * is an object, we can return a copy of the vector prime. This is ideal.
 * @param v - std::vector<float> form of the 3-value vector or vertex to be rotated
 * @return a copy of the rotated vector or vertex
 */
std::vector<float> Quaternion::rotate(std::vector<float> v) {
  return rotateVector(&v[0]);
}

/**
 * Perform a quaternion rotation on the passed vector object. Since it's
 * a primitive array, we'll enforce a size of [3] in the parameters and
 * return a copy of a std::vector<float>. See header comments on Return Types.
 * @param v - float[] form of the 3-value vector or vertex to be rotated
 * @return a copy of the rotated vector as an STL vector
 */
std::vector<float> Quaternion::rotate(float v[3]) {
  return rotateVector(v);
}

/**
 * Perform a quaternion rotation on the passed glm::vec3 object. glm::vec3 is
 * just a wrapper for a float (*)[3], which we cover explicitly above.
 * @param v - glm::vec3 form of the 3-value vector or vertex to be rotated
 * @return a copy of the rotated glm::vec3
 */
glm::vec3 Quaternion::rotate(glm::vec3 v) {
  std::vector<float> vResult = rotateVector(&v[0]);
  return glm::vec3(vResult[0], vResult[1], vResult[2]);
}

/**
 * Provides a current string representation of this quaternion.
 */
void Quaternion::toString() {
  std::cout << "[" << W << ", " << X << ", " << Y << ", " << Z << "]" << std::endl;
}

/**
 * Provides a current string representation of this quaternion's rotation
 * matrix representation.
 */
void Quaternion::matrixToString() {
  if (dirty)
    this->makeMatrix();

  std::cout << "[" << rotMatrix[0] << ", " << rotMatrix[1] << ", " << rotMatrix[2]
       << ", " << rotMatrix[3] << "]" << std::endl;
  std::cout << "|" << rotMatrix[4] << ", " << rotMatrix[5] << ", " << rotMatrix[6]
       << ", " << rotMatrix[7] << "|" << std::endl;
  std::cout << "|" << rotMatrix[8] << ", " << rotMatrix[9] << ", " << rotMatrix[10]
       << ", " << rotMatrix[11] << "|" << std::endl;
  std::cout << "[" << rotMatrix[12] << ", " << rotMatrix[13] << ", " << rotMatrix[14]
       << ", " << rotMatrix[15] << "]" << std::endl;
}

/**
 * Clears the quaternion back to the default or zero quaternion.
 */
void Quaternion::zero() {
  this->W = 1;
  this->X = 0;
  this->Y = 0;
  this->Z = 0;
  this->dirty = 1;
  if (rotMatrix.size())
    std::fill(rotMatrix.begin(), rotMatrix.end(), 0.0f);
}




/***************************
 * Operators
 */



/**
 * Add two quaternions. This won't be used often, but it is defined.
 * @param b - the right-hand operand
 * @return the sum of this and the rhs quaternion
 */
Quaternion Quaternion::operator+ (const Quaternion &b) {
  Quaternion qSum = Quaternion();

  qSum.W = this->W + b.W;
  qSum.X = this->X + b.X;
  qSum.Y = this->Y + b.Y;
  qSum.Z = this->Z + b.Z;

  qSum.normalize();

  return qSum;
}

/**
 * Multiply the following quaternion by this quaternion.
 * @param b - the right-hand operand
 * @return a copy of the quaternion product
 */
Quaternion Quaternion::operator* (const Quaternion &b) {
  Quaternion qProduct = Quaternion();
  float fW = this->W;
  float fX = this->X;
  float fY = this->Y;
  float fZ = this->Z;

  qProduct.W = (fW * b.W) - (fX * b.X) - (fY * b.Y) - (fZ * b.Z);
  qProduct.X = (fW * b.X) + (fX * b.W) + (fY * b.Z) - (fZ * b.Y);
  qProduct.Y = (fW * b.Y) - (fX * b.Z) + (fY * b.W) + (fZ * b.X);
  qProduct.Z = (fW * b.Z) + (fX * b.Y) - (fY * b.X) + (fZ * b.W);

  //qProduct.normalize();     // Needed if all quaternions are pre-normalized?

  return qProduct;
}

/**
 * Assign new values to the existing quaternion. This is necessary because
 * the above operators only return copies that will be destructed upon return.
 * @param b - the right-hand operand
 * @return this updated quaternion
 */
Quaternion& Quaternion::operator= (const Quaternion& b) {
  this->W = b.W;
  this->X = b.X;
  this->Y = b.Y;
  this->Z = b.Z;

  // Matrix representation is now out-of-date.
  this->dirty = 1;

  return *this;
}

/**
 * Add the rhs quaternion to this quaternion.
 * This won't be used often, but it is defined.
 * @param b - the right-hand operand
 * @return this updated quaternion as sum
 */
Quaternion& Quaternion::operator+= (const Quaternion &b) {
  this->W = this->W + b.W;
  this->X = this->X + b.X;
  this->Y = this->Y + b.Y;
  this->Z = this->Z + b.Z;

  this->normalize();     // Needed if all quaternions are pre-normalized?

  return *this;
}

/**
 * Make this quaternion the product of it and the rhs quaternion.
 * We can use the above operator* to retrieve a pre-normalized quaternion.
 * @param b - the right-hand operand
 * @return this updated quaternion as product
 */
Quaternion& Quaternion::operator*= (const Quaternion &b) {
  Quaternion qProduct = this->operator *(b);

  this->W = qProduct.W;
  this->X = qProduct.X;
  this->Y = qProduct.Y;
  this->Z = qProduct.Z;

  // Matrix representation is now out-of-date.
  this->dirty = 1;

  return *this;
}
