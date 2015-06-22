//
//  ShapeColliderTests.cpp
//  tests/physics/src
//
//  Created by Andrew Meadows on 02/21/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

//#include <stdio.h>
#include <iostream>
#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <AACubeShape.h>
#include <CapsuleShape.h>
#include <CollisionInfo.h>
#include <PlaneShape.h>
#include <ShapeCollider.h>
#include <SharedUtil.h>
#include <SphereShape.h>
#include <StreamUtils.h>

#include "ShapeColliderTests.h"


const glm::vec3 origin(0.0f);
static const glm::vec3 xAxis(1.0f, 0.0f, 0.0f);
static const glm::vec3 yAxis(0.0f, 1.0f, 0.0f);
static const glm::vec3 zAxis(0.0f, 0.0f, 1.0f);

QTEST_MAIN(ShapeColliderTests)

void ShapeColliderTests::initTestCase() {
    ShapeCollider::initDispatchTable();
}

void ShapeColliderTests::sphereMissesSphere() {
    // non-overlapping spheres of unequal size
    float radiusA = 7.0f;
    float radiusB = 3.0f;
    float alpha = 1.2f;
    float beta = 1.3f;
    glm::vec3 offsetDirection = glm::normalize(glm::vec3(1.0f, 2.0f, 3.0f));
    float offsetDistance = alpha * radiusA + beta * radiusB;

    SphereShape sphereA(radiusA, origin);
    SphereShape sphereB(radiusB, offsetDistance * offsetDirection);
    CollisionList collisions(16);

    // collide A to B and vice versa
    QCOMPARE(ShapeCollider::collideShapes(&sphereA, &sphereB, collisions), false);
    QCOMPARE(ShapeCollider::collideShapes(&sphereB, &sphereA, collisions), false);
    
    // Collision list should be empty
    QCOMPARE(collisions.size(), 0);
}

void ShapeColliderTests::sphereTouchesSphere() {
    // overlapping spheres of unequal size
    float radiusA = 7.0f;
    float radiusB = 3.0f;
    float alpha = 0.2f;
    float beta = 0.3f;
    glm::vec3 offsetDirection = glm::normalize(glm::vec3(1.0f, 2.0f, 3.0f));
    float offsetDistance = alpha * radiusA + beta * radiusB;
    float expectedPenetrationDistance = (1.0f - alpha) * radiusA + (1.0f - beta) * radiusB;
    glm::vec3 expectedPenetration = expectedPenetrationDistance * offsetDirection;

    SphereShape sphereA(radiusA, origin);
    SphereShape sphereB(radiusB, offsetDistance * offsetDirection);
    CollisionList collisions(16);
    int numCollisions = 0;

    // collide A to B...
    {
        QCOMPARE(ShapeCollider::collideShapes(&sphereA, &sphereB, collisions), true);
        ++numCollisions;
        
        // verify state of collisions
        QCOMPARE(collisions.size(), numCollisions);
        CollisionInfo* collision = collisions.getCollision(numCollisions - 1);
        QVERIFY(collision != nullptr);

        // penetration points from sphereA into sphereB
        QCOMPARE(collision->_penetration, expectedPenetration);

        // contactPoint is on surface of sphereA
        glm::vec3 AtoB = sphereB.getTranslation() - sphereA.getTranslation();
        glm::vec3 expectedContactPoint = sphereA.getTranslation() + radiusA * glm::normalize(AtoB);
        QCOMPARE(collision->_contactPoint, expectedContactPoint);
        
        QFUZZY_COMPARE(collision->_contactPoint, expectedContactPoint, EPSILON);
    }

    // collide B to A...
    {
        QCOMPARE(ShapeCollider::collideShapes(&sphereA, &sphereB, collisions), true);
        ++numCollisions;

        // penetration points from sphereA into sphereB
        CollisionInfo* collision = collisions.getCollision(numCollisions - 1);
        QFUZZY_COMPARE(collision->_penetration, expectedPenetration, EPSILON);

        // contactPoint is on surface of sphereA
        glm::vec3 BtoA = sphereA.getTranslation() - sphereB.getTranslation();
        glm::vec3 expectedContactPoint = sphereB.getTranslation() + radiusB * glm::normalize(BtoA);
        QFUZZY_COMPARE(collision->_contactPoint, expectedContactPoint, EPSILON);
    }
}

void ShapeColliderTests::sphereMissesCapsule() {
    // non-overlapping sphere and capsule
    float radiusA = 1.5f;
    float radiusB = 2.3f;
    float totalRadius = radiusA + radiusB;
    float halfHeightB = 1.7f;
    float axialOffset = totalRadius + 1.1f * halfHeightB;
    float radialOffset = 1.2f * radiusA + 1.3f * radiusB;

    SphereShape sphereA(radiusA);
    CapsuleShape capsuleB(radiusB, halfHeightB);

    // give the capsule some arbirary transform
    float angle = 37.8f;
    glm::vec3 axis = glm::normalize( glm::vec3(-7.0f, 2.8f, 9.3f) );
    glm::quat rotation = glm::angleAxis(angle, axis);
    glm::vec3 translation(15.1f, -27.1f, -38.6f);
    capsuleB.setRotation(rotation);
    capsuleB.setTranslation(translation);

    CollisionList collisions(16);

    // walk sphereA along the local yAxis next to, but not touching, capsuleB
    glm::vec3 localStartPosition(radialOffset, axialOffset, 0.0f);
    int numberOfSteps = 10;
    float delta = 1.3f * (totalRadius + halfHeightB) / (numberOfSteps - 1);
    for (int i = 0; i < numberOfSteps; ++i) {
        // translate sphereA into world-frame
        glm::vec3 localPosition = localStartPosition + ((float)i * delta) * yAxis;
        sphereA.setTranslation(rotation * localPosition + translation);

        // sphereA agains capsuleB and vice versa
        QCOMPARE(ShapeCollider::collideShapes(&sphereA, &capsuleB, collisions), false);
        QCOMPARE(ShapeCollider::collideShapes(&capsuleB, &sphereA, collisions), false);
    }
    
    QCOMPARE(collisions.size(), 0);
}

void ShapeColliderTests::sphereTouchesCapsule() {
    // overlapping sphere and capsule
    float radiusA = 2.0f;
    float radiusB = 1.0f;
    float totalRadius = radiusA + radiusB;
    float halfHeightB = 2.0f;
    float alpha = 0.5f;
    float beta = 0.5f;
    float radialOffset = alpha * radiusA + beta * radiusB;

    SphereShape sphereA(radiusA);
    CapsuleShape capsuleB(radiusB, halfHeightB);

    CollisionList collisions(16);
    int numCollisions = 0;

    {   // sphereA collides with capsuleB's cylindrical wall
        sphereA.setTranslation(radialOffset * xAxis);
        
        QCOMPARE(ShapeCollider::collideShapes(&sphereA, &capsuleB, collisions), true);
        ++numCollisions;

        // penetration points from sphereA into capsuleB
        CollisionInfo* collision = collisions.getCollision(numCollisions - 1);
        glm::vec3 expectedPenetration = (radialOffset - totalRadius) * xAxis;
        QFUZZY_COMPARE(collision->_penetration, expectedPenetration, EPSILON);

        // contactPoint is on surface of sphereA
        glm::vec3 expectedContactPoint = sphereA.getTranslation() - radiusA * xAxis;
        QFUZZY_COMPARE(collision->_contactPoint, expectedContactPoint, EPSILON);

        // capsuleB collides with sphereA
        QCOMPARE(ShapeCollider::collideShapes(&capsuleB, &sphereA, collisions), true);
        ++numCollisions;

        // penetration points from sphereA into capsuleB
        collision = collisions.getCollision(numCollisions - 1);
        expectedPenetration = - (radialOffset - totalRadius) * xAxis;
        if (collision->_shapeA == &sphereA) {
            // the ShapeCollider swapped the order of the shapes
            expectedPenetration *= -1.0f;
        }
        QFUZZY_COMPARE(collision->_penetration, expectedPenetration, EPSILON);
        
        // contactPoint is on surface of capsuleB
        glm::vec3 BtoA = sphereA.getTranslation() - capsuleB.getTranslation();
        glm::vec3 closestApproach = capsuleB.getTranslation() + glm::dot(BtoA, yAxis) * yAxis;
        expectedContactPoint = closestApproach + radiusB * glm::normalize(BtoA - closestApproach);
        if (collision->_shapeA == &sphereA) {
            // the ShapeCollider swapped the order of the shapes
            closestApproach = sphereA.getTranslation() - glm::dot(BtoA, yAxis) * yAxis;
            expectedContactPoint = closestApproach - radiusB * glm::normalize(BtoA - closestApproach);
        }
        QFUZZY_COMPARE(collision->_contactPoint, expectedContactPoint, EPSILON);
    }
    {   // sphereA hits end cap at axis
        glm::vec3 axialOffset = (halfHeightB + alpha * radiusA + beta * radiusB) * yAxis;
        sphereA.setTranslation(axialOffset);

        QCOMPARE(ShapeCollider::collideShapes(&sphereA, &capsuleB, collisions), true);
        ++numCollisions;

        // penetration points from sphereA into capsuleB
        CollisionInfo* collision = collisions.getCollision(numCollisions - 1);
        glm::vec3 expectedPenetration = - ((1.0f - alpha) * radiusA + (1.0f - beta) * radiusB) * yAxis;
        QFUZZY_COMPARE(collision->_penetration, expectedPenetration, EPSILON);

        // contactPoint is on surface of sphereA
        glm::vec3 expectedContactPoint = sphereA.getTranslation() - radiusA * yAxis;
        
        QFUZZY_COMPARE(collision->_contactPoint, expectedContactPoint, EPSILON);
//        inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
//        if (fabsf(inaccuracy) > EPSILON) {
//            std::cout << __FILE__ << ":" << __LINE__
//                << " ERROR: bad contactPoint: expected = " << expectedContactPoint
//                << " actual = " << collision->_contactPoint << std::endl;
//        }

        // capsuleB collides with sphereA
        if (!ShapeCollider::collideShapes(&capsuleB, &sphereA, collisions))
        {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: capsule and sphere should touch" << std::endl;
        } else {
            ++numCollisions;
        }

        // penetration points from sphereA into capsuleB
        collision = collisions.getCollision(numCollisions - 1);
        expectedPenetration = ((1.0f - alpha) * radiusA + (1.0f - beta) * radiusB) * yAxis;
        if (collision->_shapeA == &sphereA) {
            // the ShapeCollider swapped the order of the shapes
            expectedPenetration *= -1.0f;
        }
        QFUZZY_COMPARE(collision->_penetration, expectedPenetration, EPSILON);
//        inaccuracy = glm::length(collision->_penetration - expectedPenetration);
//        if (fabsf(inaccuracy) > EPSILON) {
//            std::cout << __FILE__ << ":" << __LINE__
//                << " ERROR: bad penetration: expected = " << expectedPenetration
//                << " actual = " << collision->_penetration << std::endl;
//        }

        // contactPoint is on surface of capsuleB
        glm::vec3 endPoint;
        capsuleB.getEndPoint(endPoint);
        expectedContactPoint = endPoint + radiusB * yAxis;
        if (collision->_shapeA == &sphereA) {
            // the ShapeCollider swapped the order of the shapes
            expectedContactPoint = axialOffset - radiusA * yAxis;
        }
        
        QFUZZY_COMPARE(collision->_contactPoint, expectedContactPoint, EPSILON);
//        inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
//        if (fabsf(inaccuracy) > EPSILON) {
//            std::cout << __FILE__ << ":" << __LINE__
//                << " ERROR: bad contactPoint: expected = " << expectedContactPoint
//                << " actual = " << collision->_contactPoint << std::endl;
//        }
    }
    {   // sphereA hits start cap at axis
        glm::vec3 axialOffset = - (halfHeightB + alpha * radiusA + beta * radiusB) * yAxis;
        sphereA.setTranslation(axialOffset);

        if (!ShapeCollider::collideShapes(&sphereA, &capsuleB, collisions))
        {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: sphere and capsule should touch" << std::endl;
        } else {
            ++numCollisions;
        }

        // penetration points from sphereA into capsuleB
        CollisionInfo* collision = collisions.getCollision(numCollisions - 1);
        glm::vec3 expectedPenetration = ((1.0f - alpha) * radiusA + (1.0f - beta) * radiusB) * yAxis;
        
        QFUZZY_COMPARE(collision->_penetration, expectedPenetration, EPSILON);
//        float inaccuracy = glm::length(collision->_penetration - expectedPenetration);
//        if (fabsf(inaccuracy) > EPSILON) {
//            std::cout << __FILE__ << ":" << __LINE__
//                << " ERROR: bad penetration: expected = " << expectedPenetration
//                << " actual = " << collision->_penetration << std::endl;
//        }

        // contactPoint is on surface of sphereA
        glm::vec3 expectedContactPoint = sphereA.getTranslation() + radiusA * yAxis;
        QFUZZY_COMPARE(collision->_contactPoint, expectedContactPoint, EPSILON);
//        inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
//        if (fabsf(inaccuracy) > EPSILON) {
//            std::cout << __FILE__ << ":" << __LINE__
//                << " ERROR: bad contactPoint: expected = " << expectedContactPoint
//                << " actual = " << collision->_contactPoint << std::endl;
//        }

        // capsuleB collides with sphereA
        if (!ShapeCollider::collideShapes(&capsuleB, &sphereA, collisions))
        {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: capsule and sphere should touch" << std::endl;
        } else {
            ++numCollisions;
        }

        // penetration points from sphereA into capsuleB
        collision = collisions.getCollision(numCollisions - 1);
        expectedPenetration = - ((1.0f - alpha) * radiusA + (1.0f - beta) * radiusB) * yAxis;
        if (collision->_shapeA == &sphereA) {
            // the ShapeCollider swapped the order of the shapes
            expectedPenetration *= -1.0f;
        }
        QFUZZY_COMPARE(collision->_penetration, expectedPenetration, EPSILON);
//        inaccuracy = glm::length(collision->_penetration - expectedPenetration);
//        if (fabsf(inaccuracy) > EPSILON) {
//            std::cout << __FILE__ << ":" << __LINE__
//                << " ERROR: bad penetration: expected = " << expectedPenetration
//                << " actual = " << collision->_penetration << std::endl;
//        }

        // contactPoint is on surface of capsuleB
        glm::vec3 startPoint;
        capsuleB.getStartPoint(startPoint);
        expectedContactPoint = startPoint - radiusB * yAxis;
        if (collision->_shapeA == &sphereA) {
            // the ShapeCollider swapped the order of the shapes
            expectedContactPoint = axialOffset + radiusA * yAxis;
        }
        QFUZZY_COMPARE(collision->_contactPoint, expectedContactPoint, EPSILON);
//        inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
//        if (fabsf(inaccuracy) > EPSILON) {
//            std::cout << __FILE__ << ":" << __LINE__
//                << " ERROR: bad contactPoint: expected = " << expectedContactPoint
//                << " actual = " << collision->_contactPoint << std::endl;
//        }
    }
    if (collisions.size() != numCollisions) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: expected " << numCollisions << " collisions but actual number is " << collisions.size()
            << std::endl;
    }
}

void ShapeColliderTests::capsuleMissesCapsule() {
    // non-overlapping capsules
    float radiusA = 2.0f;
    float halfHeightA = 3.0f;
    float radiusB = 3.0f;
    float halfHeightB = 4.0f;

    float totalRadius = radiusA + radiusB;
    float totalHalfLength = totalRadius + halfHeightA + halfHeightB;

    CapsuleShape capsuleA(radiusA, halfHeightA);
    CapsuleShape capsuleB(radiusB, halfHeightB);

    CollisionList collisions(16);

    // side by side
    capsuleB.setTranslation((1.01f * totalRadius) * xAxis);
    QCOMPARE(ShapeCollider::collideShapes(&capsuleA, &capsuleB, collisions), false);
    QCOMPARE(ShapeCollider::collideShapes(&capsuleB, &capsuleA, collisions), false);
//    if (ShapeCollider::collideShapes(&capsuleA, &capsuleB, collisions))
//    {
//        std::cout << __FILE__ << ":" << __LINE__
//            << " ERROR: capsule and capsule should NOT touch" << std::endl;
//    }
//    if (ShapeCollider::collideShapes(&capsuleB, &capsuleA, collisions))
//    {
//        std::cout << __FILE__ << ":" << __LINE__
//            << " ERROR: capsule and capsule should NOT touch" << std::endl;
//    }

    // end to end
    capsuleB.setTranslation((1.01f * totalHalfLength) * xAxis);
    QCOMPARE(ShapeCollider::collideShapes(&capsuleA, &capsuleB, collisions), false);
    QCOMPARE(ShapeCollider::collideShapes(&capsuleB, &capsuleA, collisions), false);
//    if (ShapeCollider::collideShapes(&capsuleA, &capsuleB, collisions))
//    {
//        std::cout << __FILE__ << ":" << __LINE__
//            << " ERROR: capsule and capsule should NOT touch" << std::endl;
//    }
//    if (ShapeCollider::collideShapes(&capsuleB, &capsuleA, collisions))
//    {
//        std::cout << __FILE__ << ":" << __LINE__
//            << " ERROR: capsule and capsule should NOT touch" << std::endl;
//    }

    // rotate B and move it to the side
    glm::quat rotation = glm::angleAxis(PI_OVER_TWO, zAxis);
    capsuleB.setRotation(rotation);
    capsuleB.setTranslation((1.01f * (totalRadius + capsuleB.getHalfHeight())) * xAxis);
    
    QCOMPARE(ShapeCollider::collideShapes(&capsuleA, &capsuleB, collisions), false);
    QCOMPARE(ShapeCollider::collideShapes(&capsuleB, &capsuleA, collisions), false);
//    if (ShapeCollider::collideShapes(&capsuleA, &capsuleB, collisions))
//    {
//        std::cout << __FILE__ << ":" << __LINE__
//            << " ERROR: capsule and capsule should NOT touch" << std::endl;
//    }
//    if (ShapeCollider::collideShapes(&capsuleB, &capsuleA, collisions))
//    {
//        std::cout << __FILE__ << ":" << __LINE__
//            << " ERROR: capsule and capsule should NOT touch" << std::endl;
//    }

    QCOMPARE(collisions.size(), 0);
//    if (collisions.size() > 0) {
//        std::cout << __FILE__ << ":" << __LINE__
//            << " ERROR: expected empty collision list but size is " << collisions.size() << std::endl;
//    }
}

void ShapeColliderTests::capsuleTouchesCapsule() {
    // overlapping capsules
    float radiusA = 2.0f;
    float halfHeightA = 3.0f;
    float radiusB = 3.0f;
    float halfHeightB = 4.0f;

    float totalRadius = radiusA + radiusB;
    float totalHalfLength = totalRadius + halfHeightA + halfHeightB;

    CapsuleShape capsuleA(radiusA, halfHeightA);
    CapsuleShape capsuleB(radiusB, halfHeightB);

    CollisionList collisions(16);
    int numCollisions = 0;

    { // side by side
        capsuleB.setTranslation((0.99f * totalRadius) * xAxis);
        QCOMPARE(ShapeCollider::collideShapes(&capsuleA, &capsuleB, collisions), true);
        QCOMPARE(ShapeCollider::collideShapes(&capsuleB, &capsuleA, collisions), true);
        numCollisions += 2;
//        if (!ShapeCollider::collideShapes(&capsuleA, &capsuleB, collisions))
//        {
//            std::cout << __FILE__ << ":" << __LINE__
//                << " ERROR: capsule and capsule should touch" << std::endl;
//        } else {
//            ++numCollisions;
//        }
//        if (!ShapeCollider::collideShapes(&capsuleB, &capsuleA, collisions))
//        {
//            std::cout << __FILE__ << ":" << __LINE__
//                << " ERROR: capsule and capsule should touch" << std::endl;
//        } else {
//            ++numCollisions;
//        }
    }

    { // end to end
        capsuleB.setTranslation((0.99f * totalHalfLength) * yAxis);
        
        QCOMPARE(ShapeCollider::collideShapes(&capsuleA, &capsuleB, collisions), true);
        QCOMPARE(ShapeCollider::collideShapes(&capsuleB, &capsuleA, collisions), true);
        numCollisions += 2;
//        if (!ShapeCollider::collideShapes(&capsuleA, &capsuleB, collisions))
//        {
//            std::cout << __FILE__ << ":" << __LINE__
//                << " ERROR: capsule and capsule should touch" << std::endl;
//        } else {
//            ++numCollisions;
//        }
//        if (!ShapeCollider::collideShapes(&capsuleB, &capsuleA, collisions))
//        {
//            std::cout << __FILE__ << ":" << __LINE__
//                << " ERROR: capsule and capsule should touch" << std::endl;
//        } else {
//            ++numCollisions;
//        }
    }

    { // rotate B and move it to the side
        glm::quat rotation = glm::angleAxis(PI_OVER_TWO, zAxis);
        capsuleB.setRotation(rotation);
        capsuleB.setTranslation((0.99f * (totalRadius + capsuleB.getHalfHeight())) * xAxis);

        QCOMPARE(ShapeCollider::collideShapes(&capsuleA, &capsuleB, collisions), true);
        QCOMPARE(ShapeCollider::collideShapes(&capsuleB, &capsuleA, collisions), true);
        numCollisions += 2;
//        if (!ShapeCollider::collideShapes(&capsuleA, &capsuleB, collisions))
//        {
//            std::cout << __FILE__ << ":" << __LINE__
//                << " ERROR: capsule and capsule should touch" << std::endl;
//        } else {
//            ++numCollisions;
//        }
//        if (!ShapeCollider::collideShapes(&capsuleB, &capsuleA, collisions))
//        {
//            std::cout << __FILE__ << ":" << __LINE__
//                << " ERROR: capsule and capsule should touch" << std::endl;
//        } else {
//            ++numCollisions;
//        }
    }

    { // again, but this time check collision details
        float overlap = 0.1f;
        glm::quat rotation = glm::angleAxis(PI_OVER_TWO, zAxis);
        capsuleB.setRotation(rotation);
        glm::vec3 positionB = ((totalRadius + capsuleB.getHalfHeight()) - overlap) * xAxis;
        capsuleB.setTranslation(positionB);
    
        // capsuleA vs capsuleB
        QCOMPARE(ShapeCollider::collideShapes(&capsuleA, &capsuleB, collisions), true);
        ++numCollisions;
//        if (!ShapeCollider::collideShapes(&capsuleA, &capsuleB, collisions))
//        {
//            std::cout << __FILE__ << ":" << __LINE__
//                << " ERROR: capsule and capsule should touch" << std::endl;
//        } else {
//            ++numCollisions;
//        }

        CollisionInfo* collision = collisions.getCollision(numCollisions - 1);
        glm::vec3 expectedPenetration = overlap * xAxis;
        
        QFUZZY_COMPARE(collision->_penetration, expectedPenetration, EPSILON);
//        float inaccuracy = glm::length(collision->_penetration - expectedPenetration);
//        if (fabsf(inaccuracy) > EPSILON) {
//            std::cout << __FILE__ << ":" << __LINE__
//                << " ERROR: bad penetration: expected = " << expectedPenetration
//                << " actual = " << collision->_penetration << std::endl;
//        }

        glm::vec3 expectedContactPoint = capsuleA.getTranslation() + radiusA * xAxis;
        QFUZZY_COMPARE(collision->_contactPoint, expectedContactPoint, EPSILON);
//        inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
//        if (fabsf(inaccuracy) > EPSILON) {
//            std::cout << __FILE__ << ":" << __LINE__
//                << " ERROR: bad contactPoint: expected = " << expectedContactPoint
//                << " actual = " << collision->_contactPoint << std::endl;
//        }

        // capsuleB vs capsuleA
        QCOMPARE(ShapeCollider::collideShapes(&capsuleB, &capsuleA, collisions), true);
        ++numCollisions;
//        if (!ShapeCollider::collideShapes(&capsuleB, &capsuleA, collisions))
//        {
//            std::cout << __FILE__ << ":" << __LINE__
//                << " ERROR: capsule and capsule should touch" << std::endl;
//        } else {
//            ++numCollisions;
//        }
        collision = collisions.getCollision(numCollisions - 1);
        expectedPenetration = - overlap * xAxis;
        QFUZZY_COMPARE(collision->_penetration, expectedPenetration, EPSILON);
//        inaccuracy = glm::length(collision->_penetration - expectedPenetration);
//        if (fabsf(inaccuracy) > EPSILON) {
//            std::cout << __FILE__ << ":" << __LINE__
//                << " ERROR: bad penetration: expected = " << expectedPenetration
//                << " actual = " << collision->_penetration << std::endl;
//        }

        expectedContactPoint = capsuleB.getTranslation() - (radiusB + halfHeightB) * xAxis;
        QFUZZY_COMPARE(collision->_contactPoint, expectedContactPoint, EPSILON);
//        inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
//        if (fabsf(inaccuracy) > EPSILON) {
//            std::cout << __FILE__ << ":" << __LINE__
//                << " ERROR: bad contactPoint: expected = " << expectedContactPoint
//                << " actual = " << collision->_contactPoint << std::endl;
//        }
    }

    { // collide cylinder wall against cylinder wall
        float overlap = 0.137f;
        float shift = 0.317f * halfHeightA;
        glm::quat rotation = glm::angleAxis(PI_OVER_TWO, zAxis);
        capsuleB.setRotation(rotation);
        glm::vec3 positionB = (totalRadius - overlap) * zAxis + shift * yAxis;
        capsuleB.setTranslation(positionB);

        // capsuleA vs capsuleB
        QCOMPARE(ShapeCollider::collideShapes(&capsuleA, &capsuleB, collisions), true);
//        if (!ShapeCollider::collideShapes(&capsuleA, &capsuleB, collisions))
//        {
//            std::cout << __FILE__ << ":" << __LINE__
//                << " ERROR: capsule and capsule should touch" << std::endl;
//        } else {
//            ++numCollisions;
//        }

        CollisionInfo* collision = collisions.getCollision(numCollisions - 1);
        glm::vec3 expectedPenetration = overlap * zAxis;
        QFUZZY_COMPARE(collision->_penetration, expectedPenetration, EPSILON);
//        float inaccuracy = glm::length(collision->_penetration - expectedPenetration);
//        if (fabsf(inaccuracy) > EPSILON) {
//            std::cout << __FILE__ << ":" << __LINE__
//                << " ERROR: bad penetration: expected = " << expectedPenetration
//                << " actual = " << collision->_penetration << std::endl;
//        }

        glm::vec3 expectedContactPoint = capsuleA.getTranslation() + radiusA * zAxis + shift * yAxis;
        QFUZZY_COMPARE(collision->_contactPoint, expectedContactPoint, EPSILON);
//        inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
//        if (fabsf(inaccuracy) > EPSILON) {
//            std::cout << __FILE__ << ":" << __LINE__
//                << " ERROR: bad contactPoint: expected = " << expectedContactPoint
//                << " actual = " << collision->_contactPoint << std::endl;
//        }
    }
}

void ShapeColliderTests::sphereMissesAACube() {
    CollisionList collisions(16);

    float sphereRadius = 1.0f;
    glm::vec3 sphereCenter(0.0f);
    
    glm::vec3 cubeCenter(1.5f, 0.0f, 0.0f);

    float cubeSide = 2.0f;

    glm::vec3 faceNormals[] = {xAxis, yAxis, zAxis};
    int numDirections = 3;

    float offset = 2.0f * EPSILON;

    // faces
    for (int i = 0; i < numDirections; ++i) {
        for (float sign = -1.0f; sign < 2.0f; sign += 2.0f) {
            glm::vec3 faceNormal = sign * faceNormals[i];

            sphereCenter = cubeCenter + (0.5f * cubeSide + sphereRadius + offset) * faceNormal;

            CollisionInfo* collision = ShapeCollider::sphereVsAACubeHelper(sphereCenter, sphereRadius,
                    cubeCenter, cubeSide, collisions);

            if (collision) {
                QFAIL_WITH_MESSAGE("sphere should NOT collide with cube face.\n\t\t"
                                   << "faceNormal = " << faceNormal);
//                std::cout << __FILE__ << ":" << __LINE__ << " ERROR: sphere should NOT collide with cube face."
//                    << "  faceNormal = " << faceNormal << std::endl;
            }
        }
    }

    // edges
    int numSteps = 5;
    // loop over each face...
    for (int i = 0; i < numDirections; ++i) {
        for (float faceSign = -1.0f; faceSign < 2.0f; faceSign += 2.0f) {
            glm::vec3 faceNormal = faceSign * faceNormals[i];

            // loop over each neighboring face...
            for (int j = (i + 1) % numDirections; j != i; j = (j + 1) % numDirections) {
                // Compute the index to the third direction, which points perpendicular to both the face
                // and the neighbor face.
                int k = (j + 1) % numDirections;
                if (k == i) {
                    k = (i + 1) % numDirections;
                }
                glm::vec3 thirdNormal = faceNormals[k];

                for (float neighborSign = -1.0f; neighborSign < 2.0f; neighborSign += 2.0f) {
                    collisions.clear();
                    glm::vec3 neighborNormal = neighborSign * faceNormals[j];

                    // combine the face and neighbor normals to get the edge normal
                    glm::vec3 edgeNormal = glm::normalize(faceNormal + neighborNormal);
                    // Step the sphere along the edge in the direction of thirdNormal, starting at one corner and
                    // moving to the other.  Test the penetration (invarient) and contact (changing) at each point.
                    float delta = cubeSide / (float)(numSteps - 1);
                    glm::vec3 startPosition = cubeCenter + (0.5f * cubeSide) * (faceNormal + neighborNormal - thirdNormal);
                    for (int m = 0; m < numSteps; ++m) {
                        sphereCenter = startPosition + ((float)m * delta) * thirdNormal + (sphereRadius + offset) * edgeNormal;

                        CollisionInfo* collision = ShapeCollider::sphereVsAACubeHelper(sphereCenter, sphereRadius,
                                cubeCenter, cubeSide, collisions);

                        if (collision) {
                            QFAIL_WITH_MESSAGE("sphere should NOT collide with cube edge.\n\t\t"
                                               << "edgeNormal = " << edgeNormal);
//                            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: sphere should NOT collide with cube edge."
//                                << "  edgeNormal = " << edgeNormal << std::endl;
                        }
                    }

                }
            }
        }
    }

    // corners
    for (float firstSign = -1.0f; firstSign < 2.0f; firstSign += 2.0f) {
        glm::vec3 firstNormal = firstSign * faceNormals[0];
        for (float secondSign = -1.0f; secondSign < 2.0f; secondSign += 2.0f) {
            glm::vec3 secondNormal = secondSign * faceNormals[1];
            for (float thirdSign = -1.0f; thirdSign < 2.0f; thirdSign += 2.0f) {
                collisions.clear();
                glm::vec3 thirdNormal = thirdSign * faceNormals[2];

                // the cornerNormal is the normalized sum of the three faces
                glm::vec3 cornerNormal = glm::normalize(firstNormal + secondNormal + thirdNormal);

                // compute a direction that is slightly offset from cornerNormal
                glm::vec3 perpAxis = glm::normalize(glm::cross(cornerNormal, firstNormal));
                glm::vec3 nearbyAxis = glm::normalize(cornerNormal + 0.3f * perpAxis);

                // swing the sphere on a small cone that starts at the corner and is centered on the cornerNormal
                float delta = TWO_PI / (float)(numSteps - 1);
                for (int i = 0; i < numSteps; i++) {
                    float angle = (float)i * delta;
                    glm::quat rotation = glm::angleAxis(angle, cornerNormal);
                    glm::vec3 offsetAxis = rotation * nearbyAxis;
                    sphereCenter = cubeCenter + (SQUARE_ROOT_OF_3 * 0.5f * cubeSide) * cornerNormal + (sphereRadius + offset) * offsetAxis;

                    CollisionInfo* collision = ShapeCollider::sphereVsAACubeHelper(sphereCenter, sphereRadius,
                            cubeCenter, cubeSide, collisions);

                    if (collision) {
                        
                        QFAIL_WITH_MESSAGE("sphere should NOT collide with cube corner\n\t\t" <<
                                           "cornerNormal = " << cornerNormal);
//                        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: sphere should NOT collide with cube corner."
//                            << "cornerNormal = " << cornerNormal << std::endl;
                        break;
                    }
                }
            }
        }
    }
}

void ShapeColliderTests::sphereTouchesAACubeFaces() {
    CollisionList collisions(16);

    float sphereRadius = 1.13f;
    glm::vec3 sphereCenter(0.0f);

    glm::vec3 cubeCenter(1.23f, 4.56f, 7.89f);
    float cubeSide = 4.34f;

    glm::vec3 faceNormals[] = {xAxis, yAxis, zAxis};
    int numDirections = 3;

    for (int i = 0; i < numDirections; ++i) {
        // loop over both sides of cube positive and negative
        for (float sign = -1.0f; sign < 2.0f; sign += 2.0f) {
            glm::vec3 faceNormal = sign * faceNormals[i];
            // outside
            {
                collisions.clear();
                float overlap = 0.25f * sphereRadius;
                float parallelOffset = 0.5f * cubeSide + sphereRadius - overlap;
                float perpOffset = 0.25f * cubeSide;
                glm::vec3 expectedPenetration  = - overlap * faceNormal;

                // We rotate the position of the sphereCenter about a circle on the cube face so that
                // it hits the same face in multiple spots.  The penetration should be invarient for
                // all collisions.
                float delta = TWO_PI / 4.0f;
                for (float angle = 0; angle < TWO_PI + EPSILON; angle += delta) {
                    glm::quat rotation = glm::angleAxis(angle, faceNormal);
                    glm::vec3 perpAxis = rotation * faceNormals[(i + 1) % numDirections];

                    sphereCenter = cubeCenter + parallelOffset * faceNormal + perpOffset * perpAxis;

                    CollisionInfo* collision = ShapeCollider::sphereVsAACubeHelper(sphereCenter, sphereRadius,
                            cubeCenter, cubeSide, collisions);

                    if (!collision) {
                        
                        QFAIL_WITH_MESSAGE("sphere should collide outside cube face\n\t\t" <<
                                           "faceNormal = " << faceNormal);
//                        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: sphere should collide outside cube face."
//                            << " faceNormal = " << faceNormal
//                            << std::endl;
                        break;
                    }

                    QFUZZY_COMPARE(collision->_penetration, expectedPenetration, EPSILON);
//                    if (glm::distance(expectedPenetration, collision->_penetration) > EPSILON) {
//                        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: penetration = " << collision->_penetration
//                            << "  expected " << expectedPenetration << "  faceNormal = " << faceNormal << std::endl;
//                    }

                    
                    glm::vec3 expectedContact = sphereCenter - sphereRadius * faceNormal;
                    QFUZZY_COMPARE(collision->_contactPoint, expectedContact, EPSILON);
//                    if (glm::distance(expectedContact, collision->_contactPoint) > EPSILON) {
//                        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: contactaPoint = " << collision->_contactPoint
//                            << "  expected " << expectedContact << "  faceNormal = " << faceNormal << std::endl;
//                    }

                    QCOMPARE(collision->getShapeA(), (Shape*)nullptr);
//                    if (collision->getShapeA()) {
//                        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: collision->_shapeA should be NULL" << std::endl;
//                    }
                    QCOMPARE(collision->getShapeB(), (Shape*)nullptr);
//                    if (collision->getShapeB()) {
//                        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: collision->_shapeB should be NULL" << std::endl;
//                    }
                }
            }

            // inside
            {
                collisions.clear();
                float overlap = 1.25f * sphereRadius;
                float sphereOffset = 0.5f * cubeSide + sphereRadius - overlap;
                sphereCenter = cubeCenter + sphereOffset * faceNormal;

                CollisionInfo* collision = ShapeCollider::sphereVsAACubeHelper(sphereCenter, sphereRadius,
                        cubeCenter, cubeSide, collisions);

                if (!collision) {
                    QFAIL_WITH_MESSAGE("sphere should collide inside cube face.\n\t\t"
                                       << "faceNormal = " << faceNormal);
//                    std::cout << __FILE__ << ":" << __LINE__ << " ERROR: sphere should collide inside cube face."
//                        << "  faceNormal = " << faceNormal << std::endl;
                    break;
                }

                glm::vec3 expectedPenetration  = - overlap * faceNormal;
                QFUZZY_COMPARE(collision->_penetration, expectedPenetration, EPSILON);
//                if (glm::distance(expectedPenetration, collision->_penetration) > EPSILON) {
//                    std::cout << __FILE__ << ":" << __LINE__ << " ERROR: penetration = " << collision->_penetration
//                        << "  expected " << expectedPenetration << "  faceNormal = " << faceNormal << std::endl;
//                }

                glm::vec3 expectedContact = sphereCenter - sphereRadius * faceNormal;
                QFUZZY_COMPARE(collision->_contactPoint, expectedContact, EPSILON);
//                if (glm::distance(expectedContact, collision->_contactPoint) > EPSILON) {
//                    std::cout << __FILE__ << ":" << __LINE__ << " ERROR: contactaPoint = " << collision->_contactPoint
//                        << "  expected " << expectedContact << "  faceNormal = " << faceNormal << std::endl;
//                }
            }
        }
    }
}

void ShapeColliderTests::sphereTouchesAACubeEdges() {
    CollisionList collisions(20);

    float sphereRadius = 1.37f;
    glm::vec3 sphereCenter(0.0f);

    glm::vec3 cubeCenter(1.23f, 4.56f, 7.89f);
    float cubeSide = 2.98f;

    float overlap = 0.25f * sphereRadius;
    int numSteps = 5;

    glm::vec3 faceNormals[] = {xAxis, yAxis, zAxis};
    int numDirections = 3;

    // loop over each face...
    for (int i = 0; i < numDirections; ++i) {
        for (float faceSign = -1.0f; faceSign < 2.0f; faceSign += 2.0f) {
            glm::vec3 faceNormal = faceSign * faceNormals[i];

            // loop over each neighboring face...
            for (int j = (i + 1) % numDirections; j != i; j = (j + 1) % numDirections) {
                // Compute the index to the third direction, which points perpendicular to both the face
                // and the neighbor face.
                int k = (j + 1) % numDirections;
                if (k == i) {
                    k = (i + 1) % numDirections;
                }
                glm::vec3 thirdNormal = faceNormals[k];

                for (float neighborSign = -1.0f; neighborSign < 2.0f; neighborSign += 2.0f) {
                    collisions.clear();
                    glm::vec3 neighborNormal = neighborSign * faceNormals[j];

                    // combine the face and neighbor normals to get the edge normal
                    glm::vec3 edgeNormal = glm::normalize(faceNormal + neighborNormal);

                    // Step the sphere along the edge in the direction of thirdNormal, starting at one corner and
                    // moving to the other.  Test the penetration (invarient) and contact (changing) at each point.
                    glm::vec3 expectedPenetration  = - overlap * edgeNormal;
                    float delta = cubeSide / (float)(numSteps - 1);
                    glm::vec3 startPosition = cubeCenter + (0.5f * cubeSide) * (faceNormal + neighborNormal - thirdNormal);
                    for (int m = 0; m < numSteps; ++m) {
                        sphereCenter = startPosition + ((float)m * delta) * thirdNormal + (sphereRadius - overlap) * edgeNormal;

                        CollisionInfo* collision = ShapeCollider::sphereVsAACubeHelper(sphereCenter, sphereRadius,
                                cubeCenter, cubeSide, collisions);

                        if (!collision) {
                            QFAIL_WITH_MESSAGE("sphere should collide with cube edge.\n\t\t"
                                               << "edgeNormal = " << edgeNormal);
//                            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: sphere should collide with cube edge."
//                                << "  edgeNormal = " << edgeNormal << std::endl;
                            break;
                        }
                        QFUZZY_COMPARE(collision->_penetration, expectedPenetration, EPSILON);
//                        if (glm::distance(expectedPenetration, collision->_penetration) > EPSILON) {
//                            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: penetration = " << collision->_penetration
//                                << "  expected " << expectedPenetration << "  edgeNormal = " << edgeNormal << std::endl;
//                        }

                        glm::vec3 expectedContact = sphereCenter - sphereRadius * edgeNormal;
                        QFUZZY_COMPARE(collision->_contactPoint, expectedContact, EPSILON);
//                        if (glm::distance(expectedContact, collision->_contactPoint) > EPSILON) {
//                            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: contactaPoint = " << collision->_contactPoint
//                                << "  expected " << expectedContact << "  edgeNormal = " << edgeNormal << std::endl;
//                        }
                    }
                }
            }
        }
    }
}

void ShapeColliderTests::sphereTouchesAACubeCorners() {
    CollisionList collisions(20);

    float sphereRadius = 1.37f;
    glm::vec3 sphereCenter(0.0f);

    glm::vec3 cubeCenter(1.23f, 4.56f, 7.89f);
    float cubeSide = 2.98f;

    float overlap = 0.25f * sphereRadius;
    int numSteps = 5;

    glm::vec3 faceNormals[] = {xAxis, yAxis, zAxis};

    for (float firstSign = -1.0f; firstSign < 2.0f; firstSign += 2.0f) {
        glm::vec3 firstNormal = firstSign * faceNormals[0];
        for (float secondSign = -1.0f; secondSign < 2.0f; secondSign += 2.0f) {
            glm::vec3 secondNormal = secondSign * faceNormals[1];
            for (float thirdSign = -1.0f; thirdSign < 2.0f; thirdSign += 2.0f) {
                collisions.clear();
                glm::vec3 thirdNormal = thirdSign * faceNormals[2];

                // the cornerNormal is the normalized sum of the three faces
                glm::vec3 cornerNormal = glm::normalize(firstNormal + secondNormal + thirdNormal);

                // compute a direction that is slightly offset from cornerNormal
                glm::vec3 perpAxis = glm::normalize(glm::cross(cornerNormal, firstNormal));
                glm::vec3 nearbyAxis = glm::normalize(cornerNormal + 0.1f * perpAxis);

                // swing the sphere on a small cone that starts at the corner and is centered on the cornerNormal
                float delta = TWO_PI / (float)(numSteps - 1);
                for (int i = 0; i < numSteps; i++) {
                    float angle = (float)i * delta;
                    glm::quat rotation = glm::angleAxis(angle, cornerNormal);
                    glm::vec3 offsetAxis = rotation * nearbyAxis;
                    sphereCenter = cubeCenter + (SQUARE_ROOT_OF_3 * 0.5f * cubeSide) * cornerNormal + (sphereRadius - overlap) * offsetAxis;

                    CollisionInfo* collision = ShapeCollider::sphereVsAACubeHelper(sphereCenter, sphereRadius,
                            cubeCenter, cubeSide, collisions);

                    if (!collision) {
                        QFAIL_WITH_MESSAGE("sphere should collide with cube corner.\n\t\t"
                            << "cornerNormal = " << cornerNormal);
                        break;
                    }

                    glm::vec3 expectedPenetration = - overlap * offsetAxis;
                    QFUZZY_COMPARE(collision->_penetration, expectedPenetration, EPSILON);
//                    if (glm::distance(expectedPenetration, collision->_penetration) > EPSILON) {
//                        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: penetration = " << collision->_penetration
//                            << "  expected " << expectedPenetration << "  cornerNormal = " << cornerNormal << std::endl;
//                    }

                    glm::vec3 expectedContact = sphereCenter - sphereRadius * offsetAxis;
                    QFUZZY_COMPARE(collision->_contactPoint, expectedContact, EPSILON);
//                    if (glm::distance(expectedContact, collision->_contactPoint) > EPSILON) {
//                        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: contactaPoint = " << collision->_contactPoint
//                            << "  expected " << expectedContact << "  cornerNormal = " << cornerNormal << std::endl;
//                    }
                }
            }
        }
    }
}

void ShapeColliderTests::capsuleMissesAACube() {
    CollisionList collisions(16);

    float capsuleRadius = 1.0f;

    glm::vec3 cubeCenter(1.23f, 4.56f, 7.89f);
    float cubeSide = 2.0f;
    AACubeShape cube(cubeSide, cubeCenter);

    glm::vec3 faceNormals[] = {xAxis, yAxis, zAxis};
    int numDirections = 3;

    float offset = 2.0f * EPSILON;

    // capsule caps miss cube faces
    for (int i = 0; i < numDirections; ++i) {
        for (float sign = -1.0f; sign < 2.0f; sign += 2.0f) {
            glm::vec3 faceNormal = sign * faceNormals[i];
            glm::vec3 secondNormal = faceNormals[(i + 1) % numDirections];
            glm::vec3 thirdNormal = faceNormals[(i + 2) % numDirections];

            // pick a random point somewhere above the face
            glm::vec3 startPoint = cubeCenter + (cubeSide + capsuleRadius) * faceNormal +
                (cubeSide * (randFloat() - 0.5f)) * secondNormal +
                (cubeSide * (randFloat() - 0.5f)) * thirdNormal;

            // pick a second random point slightly more than one radius above the face
            glm::vec3 endPoint = cubeCenter + (0.5f * cubeSide + capsuleRadius + offset) * faceNormal +
                (cubeSide * (randFloat() - 0.5f)) * secondNormal +
                (cubeSide * (randFloat() - 0.5f)) * thirdNormal;

            // randomly swap the points so capsule axis may point toward or away from face
            if (randFloat() > 0.5f) {
                glm::vec3 temp = startPoint;
                startPoint = endPoint;
                endPoint = temp;
            }

            // create a capsule between the points
            CapsuleShape capsule(capsuleRadius, startPoint, endPoint);

            // collide capsule with cube
            QCOMPARE(ShapeCollider::capsuleVsAACube(&capsule, &cube, collisions), false);
//            if (ShapeCollider::capsuleVsAACube(&capsule, &cube, collisions)) {
//                std::cout << __FILE__ << ":" << __LINE__ << " ERROR: capsule should NOT collide with cube face."
//                    << "  faceNormal = " << faceNormal << std::endl;
//            }
        }
    }

    // capsule caps miss cube edges
    // loop over each face...
    for (int i = 0; i < numDirections; ++i) {
        for (float faceSign = -1.0f; faceSign < 2.0f; faceSign += 2.0f) {
            glm::vec3 faceNormal = faceSign * faceNormals[i];

            // loop over each neighboring face...
            for (int j = (i + 1) % numDirections; j != i; j = (j + 1) % numDirections) {
                // Compute the index to the third direction, which points perpendicular to both the face
                // and the neighbor face.
                int k = (j + 1) % numDirections;
                if (k == i) {
                    k = (i + 1) % numDirections;
                }
                glm::vec3 thirdNormal = faceNormals[k];

                collisions.clear();
                for (float neighborSign = -1.0f; neighborSign < 2.0f; neighborSign += 2.0f) {
                    glm::vec3 neighborNormal = neighborSign * faceNormals[j];

                    // combine the face and neighbor normals to get the edge normal
                    glm::vec3 edgeNormal = glm::normalize(faceNormal + neighborNormal);

                    // pick a random point somewhere above the edge
                    glm::vec3 startPoint = cubeCenter + (SQUARE_ROOT_OF_2 * cubeSide + capsuleRadius) * edgeNormal +
                        (cubeSide * (randFloat() - 0.5f)) * thirdNormal;

                    // pick a second random point slightly more than one radius above the edge
                    glm::vec3 endPoint = cubeCenter + (SQUARE_ROOT_OF_2 * 0.5f * cubeSide + capsuleRadius + offset) * edgeNormal +
                        (cubeSide * (randFloat() - 0.5f)) * thirdNormal;

                    // randomly swap the points so capsule axis may point toward or away from edge
                    if (randFloat() > 0.5f) {
                        glm::vec3 temp = startPoint;
                        startPoint = endPoint;
                        endPoint = temp;
                    }

                    // create a capsule between the points
                    CapsuleShape capsule(capsuleRadius, startPoint, endPoint);

                    // collide capsule with cube
                    QCOMPARE(ShapeCollider::capsuleVsAACube(&capsule, &cube, collisions), false);
//                    if (hit) {
//                        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: capsule should NOT collide with cube face."
//                            << "  edgeNormal = " << edgeNormal << std::endl;
//                    }
                }
            }
        }
    }

    // capsule caps miss cube corners
    for (float firstSign = -1.0f; firstSign < 2.0f; firstSign += 2.0f) {
        glm::vec3 firstNormal = firstSign * faceNormals[0];
        for (float secondSign = -1.0f; secondSign < 2.0f; secondSign += 2.0f) {
            glm::vec3 secondNormal = secondSign * faceNormals[1];
            for (float thirdSign = -1.0f; thirdSign < 2.0f; thirdSign += 2.0f) {
                collisions.clear();
                glm::vec3 thirdNormal = thirdSign * faceNormals[2];

                // the cornerNormal is the normalized sum of the three faces
                glm::vec3 cornerNormal = glm::normalize(firstNormal + secondNormal + thirdNormal);

                // pick a random point somewhere above the corner
                glm::vec3 startPoint = cubeCenter + (SQUARE_ROOT_OF_3 * cubeSide + capsuleRadius) * cornerNormal +
                    (0.25f * cubeSide * (randFloat() - 0.5f)) * firstNormal +
                    (0.25f * cubeSide * (randFloat() - 0.5f)) * secondNormal +
                    (0.25f * cubeSide * (randFloat() - 0.5f)) * thirdNormal;

                // pick a second random point slightly more than one radius above the corner
                glm::vec3 endPoint = cubeCenter + (SQUARE_ROOT_OF_3 * 0.5f * cubeSide + capsuleRadius + offset) * cornerNormal;

                // randomly swap the points so capsule axis may point toward or away from corner
                if (randFloat() > 0.5f) {
                    glm::vec3 temp = startPoint;
                    startPoint = endPoint;
                    endPoint = temp;
                }

                // create a capsule between the points
                CapsuleShape capsule(capsuleRadius, startPoint, endPoint);

                // collide capsule with cube
                QCOMPARE(ShapeCollider::capsuleVsAACube(&capsule, &cube, collisions), false);
//                if (ShapeCollider::capsuleVsAACube(&capsule, &cube, collisions)) {
//                    std::cout << __FILE__ << ":" << __LINE__ << " ERROR: capsule should NOT collide with cube face."
//                        << "  cornerNormal = " << cornerNormal << std::endl;
//                }
            }
        }
    }

    // capsule sides almost hit cube edges
    // loop over each face...
    float capsuleLength = 2.0f;
    for (int i = 0; i < numDirections; ++i) {
        for (float faceSign = -1.0f; faceSign < 2.0f; faceSign += 2.0f) {
            glm::vec3 faceNormal = faceSign * faceNormals[i];

            // loop over each neighboring face...
            for (int j = (i + 1) % numDirections; j != i; j = (j + 1) % numDirections) {
                // Compute the index to the third direction, which points perpendicular to both the face
                // and the neighbor face.
                int k = (j + 1) % numDirections;
                if (k == i) {
                    k = (i + 1) % numDirections;
                }
                glm::vec3 thirdNormal = faceNormals[k];

                collisions.clear();
                for (float neighborSign = -1.0f; neighborSign < 2.0f; neighborSign += 2.0f) {
                    glm::vec3 neighborNormal = neighborSign * faceNormals[j];

                    // combine the face and neighbor normals to get the edge normal
                    glm::vec3 edgeNormal = glm::normalize(faceNormal + neighborNormal);

                    // pick a random point somewhere along the edge
                    glm::vec3 edgePoint = cubeCenter + (SQUARE_ROOT_OF_2 * 0.5f * cubeSide) * edgeNormal +
                        ((cubeSide - 2.0f * offset) * (randFloat() - 0.5f)) * thirdNormal;

                    // pick a random normal that is deflected slightly from edgeNormal
                    glm::vec3 deflectedNormal = glm::normalize(edgeNormal +
                            (0.1f * (randFloat() - 0.5f)) * faceNormal +
                            (0.1f * (randFloat() - 0.5f)) * neighborNormal);

                    // compute the axis direction, which will be perp to deflectedNormal and thirdNormal
                    glm::vec3 axisDirection = glm::normalize(glm::cross(deflectedNormal, thirdNormal));

                    // compute a point for the capsule's axis along deflection normal away from edgePoint
                    glm::vec3 axisPoint = edgePoint + (capsuleRadius + offset) * deflectedNormal;

                    // now we can compute the capsule endpoints
                    glm::vec3 endPoint = axisPoint + (0.5f * capsuleLength * randFloat()) * axisDirection;
                    glm::vec3 startPoint = axisPoint - (0.5f * capsuleLength * randFloat()) * axisDirection;

                    // create a capsule between the points
                    CapsuleShape capsule(capsuleRadius, startPoint, endPoint);

                    // collide capsule with cube
                    QCOMPARE(ShapeCollider::capsuleVsAACube(&capsule, &cube, collisions), false);
//                    if (ShapeCollider::capsuleVsAACube(&capsule, &cube, collisions)) {
//                        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: capsule should NOT collide with cube"
//                            << "  edgeNormal = " << edgeNormal << std::endl;
//                    }
                }
            }
        }
    }

    // capsule sides almost hit cube corners
    for (float firstSign = -1.0f; firstSign < 2.0f; firstSign += 2.0f) {
        glm::vec3 firstNormal = firstSign * faceNormals[0];
        for (float secondSign = -1.0f; secondSign < 2.0f; secondSign += 2.0f) {
            glm::vec3 secondNormal = secondSign * faceNormals[1];
            for (float thirdSign = -1.0f; thirdSign < 2.0f; thirdSign += 2.0f) {
                collisions.clear();
                glm::vec3 thirdNormal = thirdSign * faceNormals[2];

                // the cornerNormal is the normalized sum of the three faces
                glm::vec3 cornerNormal = glm::normalize(firstNormal + secondNormal + thirdNormal);

                // compute a penetration normal that is somewhat randomized about cornerNormal
                glm::vec3 penetrationNormal = - glm::normalize(cornerNormal +
                    (0.05f * cubeSide * (randFloat() - 0.5f)) * firstNormal +
                    (0.05f * cubeSide * (randFloat() - 0.5f)) * secondNormal +
                    (0.05f * cubeSide * (randFloat() - 0.5f)) * thirdNormal);

                // pick a random point somewhere above the corner
                glm::vec3 corner = cubeCenter + (0.5f * cubeSide) * (firstNormal + secondNormal + thirdNormal);
                glm::vec3 startPoint = corner + (3.0f * cubeSide) * cornerNormal +
                    (0.25f * cubeSide * (randFloat() - 0.5f)) * firstNormal +
                    (0.25f * cubeSide * (randFloat() - 0.5f)) * secondNormal +
                    (0.25f * cubeSide * (randFloat() - 0.5f)) * thirdNormal;

                // pick a second random point slightly less than one radius above the corner
                // with some sight perp motion
                glm::vec3 endPoint = corner - (capsuleRadius + offset) * penetrationNormal;

                // randomly swap the points so capsule axis may point toward or away from corner
                if (randFloat() > 0.5f) {
                    glm::vec3 temp = startPoint;
                    startPoint = endPoint;
                    endPoint = temp;
                }

                // create a capsule between the points
                CapsuleShape capsule(capsuleRadius, startPoint, endPoint);

                // collide capsule with cube
                QCOMPARE(ShapeCollider::capsuleVsAACube(&capsule, &cube, collisions), false);
//                if (ShapeCollider::capsuleVsAACube(&capsule, &cube, collisions)) {
//                    std::cout << __FILE__ << ":" << __LINE__ << " ERROR: capsule should NOT collide with cube"
//                        << "  cornerNormal = " << cornerNormal << std::endl;
//                }
            }
        }
    }

    // capsule sides almost hit cube faces
    // these are the steps along the capsuleAxis where we'll put the capsule endpoints
    float steps[] = { -1.0f, 2.0f, 0.25f, 0.75f, -1.0f };

    for (int i = 0; i < numDirections; ++i) {
        for (float sign = -1.0f; sign < 2.0f; sign += 2.0f) {
            glm::vec3 faceNormal = sign * faceNormals[i];
            glm::vec3 secondNormal = faceNormals[(i + 1) % numDirections];
            glm::vec3 thirdNormal = faceNormals[(i + 2) % numDirections];

            // pick two random point on opposite edges of the face
            glm::vec3 firstEdgeIntersection = cubeCenter + (0.5f * cubeSide) * (faceNormal + secondNormal) +
                (cubeSide * (randFloat() - 0.5f)) * thirdNormal;
            glm::vec3 secondEdgeIntersection = cubeCenter + (0.5f * cubeSide) * (faceNormal - secondNormal) +
                (cubeSide * (randFloat() - 0.5f)) * thirdNormal;

            // compute the un-normalized axis for the capsule
            glm::vec3 capsuleAxis = secondEdgeIntersection - firstEdgeIntersection;
            // there are three pairs in steps[]
            for (int j = 0; j < 4; j++) {
                collisions.clear();
                glm::vec3 startPoint = firstEdgeIntersection + steps[j] * capsuleAxis + (capsuleRadius + offset) * faceNormal;
                glm::vec3 endPoint = firstEdgeIntersection + steps[j + 1] * capsuleAxis + (capsuleRadius + offset) * faceNormal;

                // create a capsule between the points
                CapsuleShape capsule(capsuleRadius, startPoint, endPoint);

                // collide capsule with cube
                QCOMPARE(ShapeCollider::capsuleVsAACube(&capsule, &cube, collisions), false);
//                if (ShapeCollider::capsuleVsAACube(&capsule, &cube, collisions)) {
//                    std::cout << __FILE__ << ":" << __LINE__ << " ERROR: capsule should NOT collide with cube"
//                        << "  faceNormal = " << faceNormal << std::endl;
//                    break;
//                }
            }
        }
    }
}

void ShapeColliderTests::capsuleTouchesAACube() {
    CollisionList collisions(16);

    float capsuleRadius = 1.0f;

    glm::vec3 cubeCenter(1.23f, 4.56f, 7.89f);
    float cubeSide = 2.0f;
    AACubeShape cube(cubeSide, cubeCenter);

    glm::vec3 faceNormals[] = {xAxis, yAxis, zAxis};
    int numDirections = 3;

    float overlap = 0.25f * capsuleRadius;
    float allowableError = 10.0f * EPSILON;

    // capsule caps hit cube faces
    for (int i = 0; i < numDirections; ++i) {
        for (float sign = -1.0f; sign < 2.0f; sign += 2.0f) {
            glm::vec3 faceNormal = sign * faceNormals[i];
            glm::vec3 secondNormal = faceNormals[(i + 1) % numDirections];
            glm::vec3 thirdNormal = faceNormals[(i + 2) % numDirections];

            // pick a random point somewhere above the face
            glm::vec3 startPoint = cubeCenter + (cubeSide + capsuleRadius) * faceNormal +
                (cubeSide * (randFloat() - 0.5f)) * secondNormal +
                (cubeSide * (randFloat() - 0.5f)) * thirdNormal;

            // pick a second random point slightly less than one radius above the face
            // (but reduce width of range by 2*overlap to prevent the penetration from
            // registering against other faces)
            glm::vec3 endPoint = cubeCenter + (0.5f * cubeSide + capsuleRadius - overlap) * faceNormal +
                ((cubeSide - 2.0f * overlap) * (randFloat() - 0.5f)) * secondNormal +
                ((cubeSide - 2.0f * overlap) * (randFloat() - 0.5f)) * thirdNormal;
            glm::vec3 collidingPoint = endPoint;

            // randomly swap the points so capsule axis may point toward or away from face
            if (randFloat() > 0.5f) {
                glm::vec3 temp = startPoint;
                startPoint = endPoint;
                endPoint = temp;
            }

            // create a capsule between the points
            CapsuleShape capsule(capsuleRadius, startPoint, endPoint);

            // collide capsule with cube
            QCOMPARE(ShapeCollider::capsuleVsAACube(&capsule, &cube, collisions), true);
//            if (!ShapeCollider::capsuleVsAACube(&capsule, &cube, collisions)) {
//                std::cout << __FILE__ << ":" << __LINE__ << " ERROR: capsule should collide with cube"
//                    << "  faceNormal = " << faceNormal << std::endl;
//                break;
//            }

            CollisionInfo* collision = collisions.getLastCollision();
            QCOMPARE(collision != nullptr, true);
//            if (!collision) {
//                std::cout << __FILE__ << ":" << __LINE__
//                    << " ERROR: null collision with faceNormal = " << faceNormal << std::endl;
//                return;
//            }

            // penetration points from capsule into cube
            glm::vec3 expectedPenetration = - overlap * faceNormal;
            QFUZZY_COMPARE(collision->_penetration, expectedPenetration, allowableError);
//            float inaccuracy = glm::length(collision->_penetration - expectedPenetration);
//            if (fabsf(inaccuracy) > allowableError) {
//                std::cout << __FILE__ << ":" << __LINE__
//                    << " ERROR: bad penetration: expected = " << expectedPenetration
//                    << " actual = " << collision->_penetration
//                    << " faceNormal = " << faceNormal
//                    << std::endl;
//            }

            // contactPoint is on surface of capsule
            glm::vec3 expectedContactPoint = collidingPoint - capsuleRadius * faceNormal;
            QFUZZY_COMPARE(collision->_contactPoint, expectedContactPoint, allowableError);
//            inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
//            if (fabsf(inaccuracy) > allowableError) {
//                std::cout << __FILE__ << ":" << __LINE__
//                    << " ERROR: bad contactPoint: expected = " << expectedContactPoint
//                    << " actual = " << collision->_contactPoint
//                    << " faceNormal = " << faceNormal
//                    << std::endl;
//            }
        }
    }

    // capsule caps hit cube edges
    // loop over each face...
    for (int i = 0; i < numDirections; ++i) {
        for (float faceSign = -1.0f; faceSign < 2.0f; faceSign += 2.0f) {
            glm::vec3 faceNormal = faceSign * faceNormals[i];

            // loop over each neighboring face...
            for (int j = (i + 1) % numDirections; j != i; j = (j + 1) % numDirections) {
                // Compute the index to the third direction, which points perpendicular to both the face
                // and the neighbor face.
                int k = (j + 1) % numDirections;
                if (k == i) {
                    k = (i + 1) % numDirections;
                }
                glm::vec3 thirdNormal = faceNormals[k];

                collisions.clear();
                for (float neighborSign = -1.0f; neighborSign < 2.0f; neighborSign += 2.0f) {
                    glm::vec3 neighborNormal = neighborSign * faceNormals[j];

                    // combine the face and neighbor normals to get the edge normal
                    glm::vec3 edgeNormal = glm::normalize(faceNormal + neighborNormal);

                    // pick a random point somewhere above the edge
                    glm::vec3 startPoint = cubeCenter + (SQUARE_ROOT_OF_2 * cubeSide + capsuleRadius) * edgeNormal +
                        (cubeSide * (randFloat() - 0.5f)) * thirdNormal;

                    // pick a second random point slightly less than one radius above the edge
                    glm::vec3 endPoint = cubeCenter + (SQUARE_ROOT_OF_2 * 0.5f * cubeSide + capsuleRadius - overlap) * edgeNormal +
                        (cubeSide * (randFloat() - 0.5f)) * thirdNormal;
                    glm::vec3 collidingPoint = endPoint;

                    // randomly swap the points so capsule axis may point toward or away from edge
                    if (randFloat() > 0.5f) {
                        glm::vec3 temp = startPoint;
                        startPoint = endPoint;
                        endPoint = temp;
                    }

                    // create a capsule between the points
                    CapsuleShape capsule(capsuleRadius, startPoint, endPoint);

                    // collide capsule with cube
                    QCOMPARE(ShapeCollider::capsuleVsAACube(&capsule, &cube, collisions), true);
//                    if (!ShapeCollider::capsuleVsAACube(&capsule, &cube, collisions)) {
//                        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: capsule should collide with cube"
//                            << "  edgeNormal = " << edgeNormal << std::endl;
//                    }

                    CollisionInfo* collision = collisions.getLastCollision();
                    QCOMPARE(collision != nullptr, true);
//                    if (!collision) {
//                        std::cout << __FILE__ << ":" << __LINE__
//                            << " ERROR: null collision with edgeNormal = " << edgeNormal << std::endl;
//                        return;
//                    }

                    // penetration points from capsule into cube
                    glm::vec3 expectedPenetration = - overlap * edgeNormal;
                    QFUZZY_COMPARE(collision->_penetration, expectedPenetration, allowableError);
//                    float inaccuracy = glm::length(collision->_penetration - expectedPenetration);
//                    if (fabsf(inaccuracy) > allowableError) {
//                        std::cout << __FILE__ << ":" << __LINE__
//                            << " ERROR: bad penetration: expected = " << expectedPenetration
//                            << " actual = " << collision->_penetration
//                            << " edgeNormal = " << edgeNormal
//                            << std::endl;
//                    }

                    // contactPoint is on surface of capsule
                    glm::vec3 expectedContactPoint = collidingPoint - capsuleRadius * edgeNormal;
                    QFUZZY_COMPARE(collision->_contactPoint, expectedContactPoint, allowableError);
//                    inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
//                    if (fabsf(inaccuracy) > allowableError) {
//                        std::cout << __FILE__ << ":" << __LINE__
//                            << " ERROR: bad contactPoint: expected = " << expectedContactPoint
//                            << " actual = " << collision->_contactPoint
//                            << " edgeNormal = " << edgeNormal
//                            << std::endl;
//                    }
                }
            }
        }
    }

    // capsule caps hit cube corners
    for (float firstSign = -1.0f; firstSign < 2.0f; firstSign += 2.0f) {
        glm::vec3 firstNormal = firstSign * faceNormals[0];
        for (float secondSign = -1.0f; secondSign < 2.0f; secondSign += 2.0f) {
            glm::vec3 secondNormal = secondSign * faceNormals[1];
            for (float thirdSign = -1.0f; thirdSign < 2.0f; thirdSign += 2.0f) {
                collisions.clear();
                glm::vec3 thirdNormal = thirdSign * faceNormals[2];

                // the cornerNormal is the normalized sum of the three faces
                glm::vec3 cornerNormal = glm::normalize(firstNormal + secondNormal + thirdNormal);

                // pick a random point somewhere above the corner
                glm::vec3 startPoint = cubeCenter + (SQUARE_ROOT_OF_3 * cubeSide + capsuleRadius) * cornerNormal +
                    (0.25f * cubeSide * (randFloat() - 0.5f)) * firstNormal +
                    (0.25f * cubeSide * (randFloat() - 0.5f)) * secondNormal +
                    (0.25f * cubeSide * (randFloat() - 0.5f)) * thirdNormal;

                // pick a second random point slightly less than one radius above the corner
                glm::vec3 endPoint = cubeCenter + (SQUARE_ROOT_OF_3 * 0.5f * cubeSide + capsuleRadius - overlap) * cornerNormal;
                glm::vec3 collidingPoint = endPoint;

                // randomly swap the points so capsule axis may point toward or away from corner
                if (randFloat() > 0.5f) {
                    glm::vec3 temp = startPoint;
                    startPoint = endPoint;
                    endPoint = temp;
                }

                // create a capsule between the points
                CapsuleShape capsule(capsuleRadius, startPoint, endPoint);

                // collide capsule with cube
                QCOMPARE(ShapeCollider::capsuleVsAACube(&capsule, &cube, collisions), true);
//                if (!ShapeCollider::capsuleVsAACube(&capsule, &cube, collisions)) {
//                    std::cout << __FILE__ << ":" << __LINE__ << " ERROR: capsule should collide with cube"
//                        << "  cornerNormal = " << cornerNormal << std::endl;
//                }

                CollisionInfo* collision = collisions.getLastCollision();
                QCOMPARE(collision != nullptr, true);
//                if (!collision) {
//                    std::cout << __FILE__ << ":" << __LINE__
//                        << " ERROR: null collision with cornerNormal = " << cornerNormal << std::endl;
//                    return;
//                }

                // penetration points from capsule into cube
                glm::vec3 expectedPenetration = - overlap * cornerNormal;
                QFUZZY_COMPARE(collision->_penetration, expectedPenetration, allowableError);
//                float inaccuracy = glm::length(collision->_penetration - expectedPenetration);
//                if (fabsf(inaccuracy) > allowableError) {
//                    std::cout << __FILE__ << ":" << __LINE__
//                        << " ERROR: bad penetration: expected = " << expectedPenetration
//                        << " actual = " << collision->_penetration
//                        << " cornerNormal = " << cornerNormal
//                        << std::endl;
//                }

                // contactPoint is on surface of capsule
                glm::vec3 expectedContactPoint = collidingPoint - capsuleRadius * cornerNormal;
                QFUZZY_COMPARE(collision->_contactPoint, expectedContactPoint, allowableError);
//                inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
//                if (fabsf(inaccuracy) > allowableError) {
//                    std::cout << __FILE__ << ":" << __LINE__
//                        << " ERROR: bad contactPoint: expected = " << expectedContactPoint
//                        << " actual = " << collision->_contactPoint
//                        << " cornerNormal = " << cornerNormal
//                        << std::endl;
//                }
            }
        }
    }

    // capsule sides hit cube edges
    // loop over each face...
    float capsuleLength = 2.0f;
    for (int i = 0; i < numDirections; ++i) {
        for (float faceSign = -1.0f; faceSign < 2.0f; faceSign += 2.0f) {
            glm::vec3 faceNormal = faceSign * faceNormals[i];

            // loop over each neighboring face...
            for (int j = (i + 1) % numDirections; j != i; j = (j + 1) % numDirections) {
                // Compute the index to the third direction, which points perpendicular to both the face
                // and the neighbor face.
                int k = (j + 1) % numDirections;
                if (k == i) {
                    k = (i + 1) % numDirections;
                }
                glm::vec3 thirdNormal = faceNormals[k];

                collisions.clear();
                for (float neighborSign = -1.0f; neighborSign < 2.0f; neighborSign += 2.0f) {
                    glm::vec3 neighborNormal = neighborSign * faceNormals[j];

                    // combine the face and neighbor normals to get the edge normal
                    glm::vec3 edgeNormal = glm::normalize(faceNormal + neighborNormal);

                    // pick a random point somewhere along the edge
                    glm::vec3 edgePoint = cubeCenter + (SQUARE_ROOT_OF_2 * 0.5f * cubeSide) * edgeNormal +
                        ((cubeSide - 2.0f * overlap) * (randFloat() - 0.5f)) * thirdNormal;

                    // pick a random normal that is deflected slightly from edgeNormal
                    glm::vec3 deflectedNormal = glm::normalize(edgeNormal +
                            (0.1f * (randFloat() - 0.5f)) * faceNormal +
                            (0.1f * (randFloat() - 0.5f)) * neighborNormal);

                    // compute the axis direction, which will be perp to deflectedNormal and thirdNormal
                    glm::vec3 axisDirection = glm::normalize(glm::cross(deflectedNormal, thirdNormal));

                    // compute a point for the capsule's axis along deflection normal away from edgePoint
                    glm::vec3 axisPoint = edgePoint + (capsuleRadius - overlap) * deflectedNormal;

                    // now we can compute the capsule endpoints
                    glm::vec3 endPoint = axisPoint + (0.5f * capsuleLength * randFloat()) * axisDirection;
                    glm::vec3 startPoint = axisPoint - (0.5f * capsuleLength * randFloat()) * axisDirection;

                    // create a capsule between the points
                    CapsuleShape capsule(capsuleRadius, startPoint, endPoint);

                    // collide capsule with cube
                    QCOMPARE(ShapeCollider::capsuleVsAACube(&capsule, &cube, collisions), true);
//                    if (!ShapeCollider::capsuleVsAACube(&capsule, &cube, collisions)) {
//                        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: capsule should collide with cube"
//                            << "  edgeNormal = " << edgeNormal << std::endl;
//                    }

                    CollisionInfo* collision = collisions.getLastCollision();
                    QCOMPARE(collision != nullptr, true);
//                    if (!collision) {
//                        std::cout << __FILE__ << ":" << __LINE__
//                            << " ERROR: null collision with edgeNormal = " << edgeNormal << std::endl;
//                        return;
//                    }

                    // penetration points from capsule into cube
                    glm::vec3 expectedPenetration = - overlap * deflectedNormal;
                    QFUZZY_COMPARE(collision->_penetration, expectedPenetration, allowableError);
//                    float inaccuracy = glm::length(collision->_penetration - expectedPenetration);
//                    if (fabsf(inaccuracy) > allowableError / capsuleLength) {
//                        std::cout << __FILE__ << ":" << __LINE__
//                            << " ERROR: bad penetration: expected = " << expectedPenetration
//                            << " actual = " << collision->_penetration
//                            << " edgeNormal = " << edgeNormal
//                            << std::endl;
//                    }

                    // contactPoint is on surface of capsule
                    glm::vec3 expectedContactPoint = axisPoint - capsuleRadius * deflectedNormal;
                    QFUZZY_COMPARE(collision->_contactPoint, expectedContactPoint, allowableError);
//                    inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
//                    if (fabsf(inaccuracy) > allowableError / capsuleLength) {
//                        std::cout << __FILE__ << ":" << __LINE__
//                            << " ERROR: bad contactPoint: expected = " << expectedContactPoint
//                            << " actual = " << collision->_contactPoint
//                            << " edgeNormal = " << edgeNormal
//                            << std::endl;
//                    }
                }
            }
        }
    }

    // capsule sides hit cube corners
    for (float firstSign = -1.0f; firstSign < 2.0f; firstSign += 2.0f) {
        glm::vec3 firstNormal = firstSign * faceNormals[0];
        for (float secondSign = -1.0f; secondSign < 2.0f; secondSign += 2.0f) {
            glm::vec3 secondNormal = secondSign * faceNormals[1];
            for (float thirdSign = -1.0f; thirdSign < 2.0f; thirdSign += 2.0f) {
                collisions.clear();
                glm::vec3 thirdNormal = thirdSign * faceNormals[2];

                // the cornerNormal is the normalized sum of the three faces
                glm::vec3 cornerNormal = glm::normalize(firstNormal + secondNormal + thirdNormal);

                // compute a penetration normal that is somewhat randomized about cornerNormal
                glm::vec3 penetrationNormal = - glm::normalize(cornerNormal +
                    (0.05f * cubeSide * (randFloat() - 0.5f)) * firstNormal +
                    (0.05f * cubeSide * (randFloat() - 0.5f)) * secondNormal +
                    (0.05f * cubeSide * (randFloat() - 0.5f)) * thirdNormal);

                // pick a random point somewhere above the corner
                glm::vec3 corner = cubeCenter + (0.5f * cubeSide) * (firstNormal + secondNormal + thirdNormal);
                glm::vec3 startPoint = corner + (3.0f * cubeSide) * cornerNormal +
                    (0.25f * cubeSide * (randFloat() - 0.5f)) * firstNormal +
                    (0.25f * cubeSide * (randFloat() - 0.5f)) * secondNormal +
                    (0.25f * cubeSide * (randFloat() - 0.5f)) * thirdNormal;

                // pick a second random point slightly less than one radius above the corner
                // with some sight perp motion
                glm::vec3 endPoint = corner - (capsuleRadius - overlap) * penetrationNormal;
                glm::vec3 collidingPoint = endPoint;

                // randomly swap the points so capsule axis may point toward or away from corner
                if (randFloat() > 0.5f) {
                    glm::vec3 temp = startPoint;
                    startPoint = endPoint;
                    endPoint = temp;
                }

                // create a capsule between the points
                CapsuleShape capsule(capsuleRadius, startPoint, endPoint);

                // collide capsule with cube
                QCOMPARE(ShapeCollider::capsuleVsAACube(&capsule, &cube, collisions), true);
//                if (!ShapeCollider::capsuleVsAACube(&capsule, &cube, collisions)) {
//                    std::cout << __FILE__ << ":" << __LINE__ << " ERROR: capsule should collide with cube"
//                        << "  cornerNormal = " << cornerNormal << std::endl;
//                }

                CollisionInfo* collision = collisions.getLastCollision();
                QCOMPARE(collision != nullptr, true);
//                if (!collision) {
//                    std::cout << __FILE__ << ":" << __LINE__
//                        << " ERROR: null collision with cornerNormal = " << cornerNormal << std::endl;
//                    return;
//                }

                // penetration points from capsule into cube
                glm::vec3 expectedPenetration = overlap * penetrationNormal;
                QFUZZY_COMPARE(collision->_penetration, expectedPenetration, allowableError);
//                float inaccuracy = glm::length(collision->_penetration - expectedPenetration);
//                if (fabsf(inaccuracy) > allowableError) {
//                    std::cout << __FILE__ << ":" << __LINE__
//                        << " ERROR: bad penetration: expected = " << expectedPenetration
//                        << " actual = " << collision->_penetration
//                        << " cornerNormal = " << cornerNormal
//                        << std::endl;
//                }

                // contactPoint is on surface of capsule
                glm::vec3 expectedContactPoint = collidingPoint + capsuleRadius * penetrationNormal;
                QFUZZY_COMPARE(collision->_contactPoint, expectedContactPoint, allowableError);
//                inaccuracy = glm::length(collision->_contactPoint - expectedContactPoint);
//                if (fabsf(inaccuracy) > allowableError) {
//                    std::cout << __FILE__ << ":" << __LINE__
//                        << " ERROR: bad contactPoint: expected = " << expectedContactPoint
//                        << " actual = " << collision->_contactPoint
//                        << " cornerNormal = " << cornerNormal
//                        << std::endl;
//                }
            }
        }
    }

    // capsule sides hit cube faces
    // these are the steps along the capsuleAxis where we'll put the capsule endpoints
    float steps[] = { -1.0f, 2.0f, 0.25f, 0.75f, -1.0f };

    for (int i = 0; i < numDirections; ++i) {
        for (float sign = -1.0f; sign < 2.0f; sign += 2.0f) {
            glm::vec3 faceNormal = sign * faceNormals[i];
            glm::vec3 secondNormal = faceNormals[(i + 1) % numDirections];
            glm::vec3 thirdNormal = faceNormals[(i + 2) % numDirections];

            // pick two random point on opposite edges of the face
            glm::vec3 firstEdgeIntersection = cubeCenter + (0.5f * cubeSide) * (faceNormal + secondNormal) +
                (cubeSide * (randFloat() - 0.5f)) * thirdNormal;
            glm::vec3 secondEdgeIntersection = cubeCenter + (0.5f * cubeSide) * (faceNormal - secondNormal) +
                (cubeSide * (randFloat() - 0.5f)) * thirdNormal;

            // compute the un-normalized axis for the capsule
            glm::vec3 capsuleAxis = secondEdgeIntersection - firstEdgeIntersection;
            // there are three pairs in steps[]
            for (int j = 0; j < 4; j++) {
                collisions.clear();
                glm::vec3 startPoint = firstEdgeIntersection + steps[j] * capsuleAxis + (capsuleRadius - overlap) * faceNormal;
                glm::vec3 endPoint = firstEdgeIntersection + steps[j + 1] * capsuleAxis + (capsuleRadius - overlap) * faceNormal;

                // create a capsule between the points
                CapsuleShape capsule(capsuleRadius, startPoint, endPoint);

                // collide capsule with cube
                QCOMPARE(ShapeCollider::capsuleVsAACube(&capsule, &cube, collisions), true);
//                if (!ShapeCollider::capsuleVsAACube(&capsule, &cube, collisions)) {
//                    std::cout << __FILE__ << ":" << __LINE__ << " ERROR: capsule should collide with cube"
//                        << "  faceNormal = " << faceNormal << std::endl;
//                    break;
//                }

                QCOMPARE(collisions.size(), 2);
//                int numCollisions = collisions.size();
//                if (numCollisions != 2) {
//                    std::cout << __FILE__ << ":" << __LINE__
//                        << " ERROR: capsule should hit cube face at two spots."
//                        << " Expected collisions size of 2 but is actually " << numCollisions
//                        << ".  faceNormal = " << faceNormal << std::endl;
//                    break;
//                }

                // compute the expected contact points
                // NOTE: whether the startPoint or endPoint are expected to collide depends the relative values
                // of the steps[] that were used to compute them above.
                glm::vec3 expectedContactPoints[2];
                if (j == 0) {
                    expectedContactPoints[0] = firstEdgeIntersection - overlap * faceNormal;
                    expectedContactPoints[1] = secondEdgeIntersection - overlap * faceNormal;
                } else if (j == 1) {
                    expectedContactPoints[0] = secondEdgeIntersection - overlap * faceNormal;
                    expectedContactPoints[1] = endPoint - capsuleRadius * faceNormal;
                } else if (j == 2) {
                    expectedContactPoints[0] = startPoint - capsuleRadius * faceNormal;
                    expectedContactPoints[1] = endPoint - capsuleRadius * faceNormal;
                } else if (j == 3) {
                    expectedContactPoints[0] = startPoint - capsuleRadius * faceNormal;
                    expectedContactPoints[1] = firstEdgeIntersection - overlap * faceNormal;
                }

                // verify each contact
                for (int k = 0; k < 2; ++k) {
                    CollisionInfo* collision = collisions.getCollision(k);
                    // penetration points from capsule into cube
                    glm::vec3 expectedPenetration = - overlap * faceNormal;
                    QFUZZY_COMPARE(collision->_penetration, expectedPenetration, allowableError);
//                    float inaccuracy = glm::length(collision->_penetration - expectedPenetration);
//                    if (fabsf(inaccuracy) > allowableError) {
//                        std::cout << __FILE__ << ":" << __LINE__
//                            << " ERROR: bad penetration: expected = " << expectedPenetration
//                            << " actual = " << collision->_penetration
//                            << " faceNormal = " << faceNormal
//                            << std::endl;
//                    }

                    // the order of the final contact points is undefined, so we
                    // figure out which expected contact point is the closest to the real one
                    // and then verify accuracy on that
                    float length0 = glm::length(collision->_contactPoint - expectedContactPoints[0]);
                    float length1 = glm::length(collision->_contactPoint - expectedContactPoints[1]);
                    glm::vec3 expectedContactPoint = (length0 < length1) ? expectedContactPoints[0] : expectedContactPoints[1];
                    // contactPoint is on surface of capsule
                    QFUZZY_COMPARE(collision->_contactPoint, expectedContactPoint, allowableError);
//                    inaccuracy = (length0 < length1) ? length0 : length1;
//                    if (fabsf(inaccuracy) > allowableError) {
//                        std::cout << __FILE__ << ":" << __LINE__
//                            << " ERROR: bad contact: expectedContactPoint[" << k << "] = " << expectedContactPoint
//                            << " actual = " << collision->_contactPoint
//                            << " faceNormal = " << faceNormal
//                            << std::endl;
//                    }
                }
            }
        }
    }
}


void ShapeColliderTests::rayHitsSphere() {
    float startDistance = 3.0f;

    float radius = 1.0f;
    glm::vec3 center(0.0f);
    SphereShape sphere(radius, center);

    // very simple ray along xAxis
    {
        RayIntersectionInfo intersection;
        intersection._rayStart = -startDistance * xAxis;
        intersection._rayDirection = xAxis;

        QCOMPARE(sphere.findRayIntersection(intersection), true);
//        if (!sphere.findRayIntersection(intersection)) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should intersect sphere" << std::endl;
//        }
        
        float expectedDistance = startDistance - radius;
        QFUZZY_COMPARE(intersection._hitDistance, expectedDistance, startDistance * EPSILON);
//        float relativeError = fabsf(intersection._hitDistance - expectedDistance) / startDistance;
//        if (relativeError > EPSILON) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray sphere intersection distance error = " << relativeError << std::endl;
//        }
        QCOMPARE(intersection._hitShape, &sphere);
//        if (intersection._hitShape != &sphere) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray intersection._hitShape should point at sphere"
//                << std::endl;
//        }
    }

    // ray along a diagonal axis
    {
        RayIntersectionInfo intersection;
        intersection._rayStart = glm::vec3(startDistance, startDistance, 0.0f);
        intersection._rayDirection = - glm::normalize(intersection._rayStart);
        QCOMPARE(sphere.findRayIntersection(intersection), true);
//        if (!sphere.findRayIntersection(intersection)) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should intersect sphere" << std::endl;
//        }

        float expectedDistance = SQUARE_ROOT_OF_2 * startDistance - radius;
        QFUZZY_COMPARE(intersection._hitDistance, expectedDistance, startDistance * EPSILON);
//        float relativeError = fabsf(intersection._hitDistance - expectedDistance) / startDistance;
//        if (relativeError > EPSILON) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray sphere intersection distance error = " << relativeError << std::endl;
//        }
    }

    // rotated and displaced ray and sphere
    {
        startDistance = 7.41f;
        radius = 3.917f;

        glm::vec3 axis = glm::normalize(glm::vec3(1.0f, 2.0f, 3.0f));
        glm::quat rotation = glm::angleAxis(0.987654321f, axis);
        glm::vec3 translation(35.7f, 2.46f, -1.97f);

        glm::vec3 unrotatedRayDirection = -xAxis;
        glm::vec3 untransformedRayStart = startDistance * xAxis;

        RayIntersectionInfo intersection;
        intersection._rayStart = rotation * (untransformedRayStart + translation);
        intersection._rayDirection = rotation * unrotatedRayDirection;

        sphere.setRadius(radius);
        sphere.setTranslation(rotation * translation);

        QCOMPARE(sphere.findRayIntersection(intersection), true);
//        if (!sphere.findRayIntersection(intersection)) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should intersect sphere" << std::endl;
//        }

        float expectedDistance = startDistance - radius;
        QFUZZY_COMPARE(intersection._hitDistance, expectedDistance, startDistance * EPSILON);
//        float relativeError = fabsf(intersection._hitDistance - expectedDistance) / startDistance;
//        if (relativeError > EPSILON) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray sphere intersection distance error = "
//                << relativeError << std::endl;
//        }
    }
}

void ShapeColliderTests::rayBarelyHitsSphere() {
    float radius = 1.0f;
    glm::vec3 center(0.0f);
    float delta = 2.0f * EPSILON;

    SphereShape sphere(radius, center);
    float startDistance = 3.0f;

    {
        RayIntersectionInfo intersection;
        intersection._rayStart = glm::vec3(-startDistance, radius - delta, 0.0f);
        intersection._rayDirection = xAxis;

        // very simple ray along xAxis
        QCOMPARE(sphere.findRayIntersection(intersection), true);
//        if (!sphere.findRayIntersection(intersection)) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should just barely hit sphere" << std::endl;
//        }
        QCOMPARE(intersection._hitShape, &sphere);
//        if (intersection._hitShape != &sphere) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray intersection._hitShape should point at sphere"
//                << std::endl;
//        }
    }

    {
        // translate and rotate the whole system...
        glm::vec3 axis = glm::normalize(glm::vec3(1.0f, 2.0f, 3.0f));
        glm::quat rotation = glm::angleAxis(0.987654321f, axis);
        glm::vec3 translation(35.7f, 0.46f, -1.97f);

        RayIntersectionInfo intersection;
        intersection._rayStart = rotation * (intersection._rayStart + translation);
        intersection._rayDirection = rotation * intersection._rayDirection;

        sphere.setTranslation(rotation * translation);

        // ...and test again
        QCOMPARE(sphere.findRayIntersection(intersection), true);
//        if (!sphere.findRayIntersection(intersection)) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should just barely hit sphere" << std::endl;
//        }
    }
}


void ShapeColliderTests::rayBarelyMissesSphere() {
    // same as the barely-hits case, but this time we move the ray away from sphere
    float radius = 1.0f;
    glm::vec3 center(0.0f);
    float delta = 2.0f * EPSILON;

    SphereShape sphere(radius, center);
    float startDistance = 3.0f;

    {
        RayIntersectionInfo intersection;
        intersection._rayStart = glm::vec3(-startDistance, radius + delta, 0.0f);
        intersection._rayDirection = xAxis;

        // very simple ray along xAxis
        QCOMPARE(sphere.findRayIntersection(intersection), true);
//        if (sphere.findRayIntersection(intersection)) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should just barely miss sphere" << std::endl;
//        }
        QCOMPARE(intersection._hitDistance, FLT_MAX);
//        if (intersection._hitDistance != FLT_MAX) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: distance should be unchanged after intersection miss"
//                << std::endl;
//        }
    }

    {
        // translate and rotate the whole system...
        float angle = 0.987654321f;
        glm::vec3 axis = glm::normalize(glm::vec3(1.0f, 2.0f, 3.0f));
        glm::quat rotation = glm::angleAxis(angle, axis);
        glm::vec3 translation(35.7f, 2.46f, -1.97f);

        RayIntersectionInfo intersection;
        intersection._rayStart = rotation * (glm::vec3(-startDistance, radius + delta, 0.0f) + translation);
        intersection._rayDirection = rotation * xAxis;
        sphere.setTranslation(rotation * translation);

        // ...and test again
        QCOMPARE(sphere.findRayIntersection(intersection), false);
//        if (sphere.findRayIntersection(intersection)) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should just barely miss sphere" << std::endl;
//        }
        QCOMPARE(intersection._hitDistance != FLT_MAX, true);
//        if (intersection._hitDistance != FLT_MAX) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: distance should be unchanged after intersection miss"
//                << std::endl;
//        }
        QCOMPARE(intersection._hitShape == nullptr, true);
//        if (intersection._hitShape != NULL) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray intersection._hitShape should be NULL" << std::endl;
//        }
    }
}

void ShapeColliderTests::rayHitsCapsule() {
    float startDistance = 3.0f;
    float radius = 1.0f;
    float halfHeight = 2.0f;
    glm::vec3 center(0.0f);
    CapsuleShape capsule(radius, halfHeight);

    // simple tests along xAxis
    { // toward capsule center
        RayIntersectionInfo intersection;
        intersection._rayStart = glm::vec3(startDistance, 0.0f, 0.0f);
        intersection._rayDirection = - xAxis;
        QCOMPARE(capsule.findRayIntersection(intersection), true);
//        if (!capsule.findRayIntersection(intersection)) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should hit capsule" << std::endl;
//        }
        float expectedDistance = startDistance - radius;
        QFUZZY_COMPARE(intersection._hitDistance, expectedDistance, startDistance * EPSILON);
//        float relativeError = fabsf(intersection._hitDistance - expectedDistance) / startDistance;
//        if (relativeError > EPSILON) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray capsule intersection distance error = "
//                << relativeError << std::endl;
//        }
        QCOMPARE(intersection._hitShape, &capsule);
//        if (intersection._hitShape != &capsule) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray intersection._hitShape should point at capsule"
//                << std::endl;
//        }
    }

    { // toward top of cylindrical wall
        RayIntersectionInfo intersection;
        intersection._rayStart = glm::vec3(startDistance, halfHeight, 0.0f);
        intersection._rayDirection = - xAxis;
        QCOMPARE(capsule.findRayIntersection(intersection), true);
//        if (!capsule.findRayIntersection(intersection)) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should hit capsule" << std::endl;
//        }
        float expectedDistance = startDistance - radius;
        QFUZZY_COMPARE(intersection._hitDistance, expectedDistance, startDistance * EPSILON);
//        float relativeError = fabsf(intersection._hitDistance - expectedDistance) / startDistance;
//        if (relativeError > EPSILON) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray capsule intersection distance error = "
//                << relativeError << std::endl;
//        }
    }

    float delta = 2.0f * EPSILON;
    { // toward top cap
        RayIntersectionInfo intersection;
        intersection._rayStart = glm::vec3(startDistance, halfHeight + delta, 0.0f);
        intersection._rayDirection = - xAxis;
        QCOMPARE(capsule.findRayIntersection(intersection), true);
//        if (!capsule.findRayIntersection(intersection)) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should hit capsule" << std::endl;
//        }
        float expectedDistance = startDistance - radius;
        QFUZZY_COMPARE(intersection._hitDistance, expectedDistance, startDistance * EPSILON);
//        float relativeError = fabsf(intersection._hitDistance - expectedDistance) / startDistance;
//        if (relativeError > EPSILON) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray capsule intersection distance error = "
//                << relativeError << std::endl;
//        }
    }

    const float EDGE_CASE_SLOP_FACTOR = 20.0f;
    { // toward tip of top cap
        RayIntersectionInfo intersection;
        intersection._rayStart = glm::vec3(startDistance, halfHeight + radius - delta, 0.0f);
        intersection._rayDirection = - xAxis;
        QCOMPARE(capsule.findRayIntersection(intersection), true);
//        if (!capsule.findRayIntersection(intersection)) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should hit capsule" << std::endl;
//        }
        float expectedDistance = startDistance - radius * sqrtf(2.0f * delta);    // using small angle approximation of cosine
//        float relativeError = fabsf(intersection._hitDistance - expectedDistance) / startDistance;
        // for edge cases we allow a LOT of error
        QFUZZY_COMPARE(intersection._hitDistance, expectedDistance, startDistance * EDGE_CASE_SLOP_FACTOR * EPSILON);
//        if (relativeError > EDGE_CASE_SLOP_FACTOR * EPSILON) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray capsule intersection distance error = "
//                << relativeError << std::endl;
//        }
    }

    { // toward tip of bottom cap
        RayIntersectionInfo intersection;
        intersection._rayStart = glm::vec3(startDistance, - halfHeight - radius + delta, 0.0f);
        intersection._rayDirection = - xAxis;
        QCOMPARE(capsule.findRayIntersection(intersection), true);
//        if (!capsule.findRayIntersection(intersection)) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should hit capsule" << std::endl;
//        }
        float expectedDistance = startDistance - radius * sqrtf(2.0f * delta);    // using small angle approximation of cosine
//        float relativeError = fabsf(intersection._hitDistance - expectedDistance) / startDistance;
        // for edge cases we allow a LOT of error
        QFUZZY_COMPARE(intersection._hitDistance, expectedDistance, startDistance * EPSILON * EDGE_CASE_SLOP_FACTOR);
//        if (relativeError > EDGE_CASE_SLOP_FACTOR * EPSILON) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray capsule intersection distance error = "
//                << relativeError << std::endl;
//        }
    }

    { // toward edge of capsule cylindrical face
        RayIntersectionInfo intersection;
        intersection._rayStart = glm::vec3(startDistance, 0.0f, radius - delta);
        intersection._rayDirection = - xAxis;
        QCOMPARE(capsule.findRayIntersection(intersection), true);
//        if (!capsule.findRayIntersection(intersection)) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should hit capsule" << std::endl;
//        }
        float expectedDistance = startDistance - radius * sqrtf(2.0f * delta);    // using small angle approximation of cosine
        float relativeError = fabsf(intersection._hitDistance - expectedDistance) / startDistance;
        // for edge cases we allow a LOT of error
        QFUZZY_COMPARE(intersection._hitDistance, expectedDistance, startDistance * EPSILON * EDGE_CASE_SLOP_FACTOR);
//        if (relativeError > EDGE_CASE_SLOP_FACTOR * EPSILON) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray capsule intersection distance error = "
//                << relativeError << std::endl;
//        }
    }
    // TODO: test at steep angles near cylinder/cap junction
}

void ShapeColliderTests::rayMissesCapsule() {
    // same as edge case hit tests, but shifted in the opposite direction
    float startDistance = 3.0f;
    float radius = 1.0f;
    float halfHeight = 2.0f;
    glm::vec3 center(0.0f);
    CapsuleShape capsule(radius, halfHeight);

    { // simple test along xAxis
        // toward capsule center
        RayIntersectionInfo intersection;
        intersection._rayStart = glm::vec3(startDistance, 0.0f, 0.0f);
        intersection._rayDirection = -xAxis;
        float delta = 2.0f * EPSILON;

        // over top cap
        intersection._rayStart.y = halfHeight + radius + delta;
        intersection._hitDistance = FLT_MAX;
        QCOMPARE(capsule.findRayIntersection(intersection), false);
//        if (capsule.findRayIntersection(intersection)) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should miss capsule" << std::endl;
//        }
        QCOMPARE(intersection._hitDistance, FLT_MAX);
//        if (intersection._hitDistance != FLT_MAX) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: distance should be unchanged after intersection miss"
//                << std::endl;
//        }

        // below bottom cap
        intersection._rayStart.y = - halfHeight - radius - delta;
        intersection._hitDistance = FLT_MAX;
        QCOMPARE(capsule.findRayIntersection(intersection), false);
//        if (capsule.findRayIntersection(intersection)) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should miss capsule" << std::endl;
//        }
        QCOMPARE(intersection._hitDistance, FLT_MAX);
//        if (intersection._hitDistance != FLT_MAX) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: distance should be unchanged after intersection miss"
//                << std::endl;
//        }

        // past edge of capsule cylindrical face
        intersection._rayStart.y = 0.0f;
        intersection._rayStart.z = radius + delta;
        intersection._hitDistance = FLT_MAX;
        QCOMPARE(capsule.findRayIntersection(intersection), false);
//        if (capsule.findRayIntersection(intersection)) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should miss capsule" << std::endl;
//        }
        QCOMPARE(intersection._hitDistance, FLT_MAX);
//        if (intersection._hitDistance != FLT_MAX) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: distance should be unchanged after intersection miss"
//                << std::endl;
//        }
        QCOMPARE(intersection._hitShape, (Shape*)nullptr);
//        if (intersection._hitShape != NULL) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray intersection._hitShape should be NULL" << std::endl;
//        }
    }
    // TODO: test at steep angles near edge
}

void ShapeColliderTests::rayHitsPlane() {
    // make a simple plane
    float planeDistanceFromOrigin = 3.579f;
    glm::vec3 planePosition(0.0f, planeDistanceFromOrigin, 0.0f);
    PlaneShape plane;
    plane.setPoint(planePosition);
    plane.setNormal(yAxis);

    // make a simple ray
    float startDistance = 1.234f;
    {
        RayIntersectionInfo intersection;
        intersection._rayStart = -startDistance * xAxis;
        intersection._rayDirection = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));

        QCOMPARE(plane.findRayIntersection(intersection), true);
//        if (!plane.findRayIntersection(intersection)) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should hit plane" << std::endl;
//        }

        float expectedDistance = SQUARE_ROOT_OF_3 * planeDistanceFromOrigin;
        
        QFUZZY_COMPARE(intersection._hitDistance, expectedDistance, planeDistanceFromOrigin * EPSILON);
//        float relativeError = fabsf(intersection._hitDistance - expectedDistance) / planeDistanceFromOrigin;
//        if (relativeError > EPSILON) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray plane intersection distance error = "
//                << relativeError << std::endl;
//        }
        QCOMPARE(intersection._hitShape, &plane);
//        if (intersection._hitShape != &plane) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray intersection._hitShape should point at plane"
//                << std::endl;
//        }
    }

    { // rotate the whole system and try again
        float angle = 37.8f;
        glm::vec3 axis = glm::normalize( glm::vec3(-7.0f, 2.8f, 9.3f) );
        glm::quat rotation = glm::angleAxis(angle, axis);

        plane.setNormal(rotation * yAxis);
        plane.setPoint(rotation * planePosition);
        RayIntersectionInfo intersection;
        intersection._rayStart = rotation * (-startDistance * xAxis);
        intersection._rayDirection = rotation * glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));

        QCOMPARE(plane.findRayIntersection(intersection), true);
//        if (!plane.findRayIntersection(intersection)) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should hit plane" << std::endl;
//        }

        float expectedDistance = SQUARE_ROOT_OF_3 * planeDistanceFromOrigin;
        QFUZZY_COMPARE(intersection._hitDistance, expectedDistance, planeDistanceFromOrigin * EPSILON);
//        float relativeError = fabsf(intersection._hitDistance - expectedDistance) / planeDistanceFromOrigin;
//        if (relativeError > EPSILON) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray plane intersection distance error = "
//                << relativeError << std::endl;
//        }
    }
}

void ShapeColliderTests::rayMissesPlane() {
    // make a simple plane
    float planeDistanceFromOrigin = 3.579f;
    glm::vec3 planePosition(0.0f, planeDistanceFromOrigin, 0.0f);
    PlaneShape plane;
    plane.setTranslation(planePosition);

    { // parallel rays should miss
        float startDistance = 1.234f;
        RayIntersectionInfo intersection;
        intersection._rayStart = glm::vec3(-startDistance, 0.0f, 0.0f);
        intersection._rayDirection = glm::normalize(glm::vec3(-1.0f, 0.0f, -1.0f));

        QCOMPARE(plane.findRayIntersection(intersection), false);
//        if (plane.findRayIntersection(intersection)) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should miss plane" << std::endl;
//        }
        QCOMPARE(intersection._hitDistance, FLT_MAX);
//        if (intersection._hitDistance != FLT_MAX) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: distance should be unchanged after intersection miss"
//                << std::endl;
//        }

        // rotate the whole system and try again
        float angle = 37.8f;
        glm::vec3 axis = glm::normalize( glm::vec3(-7.0f, 2.8f, 9.3f) );
        glm::quat rotation = glm::angleAxis(angle, axis);

        plane.setTranslation(rotation * planePosition);
        plane.setRotation(rotation);

        intersection._rayStart = rotation * intersection._rayStart;
        intersection._rayDirection = rotation * intersection._rayDirection;
        intersection._hitDistance = FLT_MAX;

        QCOMPARE(plane.findRayIntersection(intersection), false);
//        if (plane.findRayIntersection(intersection)) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should miss plane" << std::endl;
//        }
        QCOMPARE(intersection._hitDistance, FLT_MAX);
//        if (intersection._hitDistance != FLT_MAX) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: distance should be unchanged after intersection miss"
//                << std::endl;
//        }
        QCOMPARE(intersection._hitShape, (Shape*)nullptr);
//        if (intersection._hitShape != NULL) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray intersection._hitShape should be NULL" << std::endl;
//        }
    }

    { // make a simple ray that points away from plane
        float startDistance = 1.234f;

        RayIntersectionInfo intersection;
        intersection._rayStart = glm::vec3(-startDistance, 0.0f, 0.0f);
        intersection._rayDirection = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f));
        intersection._hitDistance = FLT_MAX;

        QCOMPARE(plane.findRayIntersection(intersection), false);
//        if (plane.findRayIntersection(intersection)) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should miss plane" << std::endl;
//        }
        QCOMPARE(intersection._hitDistance, FLT_MAX);
//        if (intersection._hitDistance != FLT_MAX) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: distance should be unchanged after intersection miss"
//                << std::endl;
//        }

        // rotate the whole system and try again
        float angle = 37.8f;
        glm::vec3 axis = glm::normalize( glm::vec3(-7.0f, 2.8f, 9.3f) );
        glm::quat rotation = glm::angleAxis(angle, axis);

        plane.setTranslation(rotation * planePosition);
        plane.setRotation(rotation);

        intersection._rayStart = rotation * intersection._rayStart;
        intersection._rayDirection = rotation * intersection._rayDirection;
        intersection._hitDistance = FLT_MAX;

        QCOMPARE(plane.findRayIntersection(intersection), false);
//        if (plane.findRayIntersection(intersection)) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should miss plane" << std::endl;
//        }
        QCOMPARE(intersection._hitDistance, FLT_MAX);
//        if (intersection._hitDistance != FLT_MAX) {
//            std::cout << __FILE__ << ":" << __LINE__ << " ERROR: distance should be unchanged after intersection miss"
//                << std::endl;
//        }
    }
}

void ShapeColliderTests::rayHitsAACube() {
    glm::vec3 cubeCenter(1.23f, 4.56f, 7.89f);
    float cubeSide = 2.127f;
    AACubeShape cube(cubeSide, cubeCenter);

    float rayOffset = 3.796f;

    glm::vec3 faceNormals[] = {xAxis, yAxis, zAxis};
    int numDirections = 3;
    int numRayCasts = 5;

    for (int i = 0; i < numDirections; ++i) {
        for (float sign = -1.0f; sign < 2.0f; sign += 2.0f) {
            glm::vec3 faceNormal = sign * faceNormals[i];
            glm::vec3 secondNormal = faceNormals[(i + 1) % numDirections];
            glm::vec3 thirdNormal = faceNormals[(i + 2) % numDirections];

            // pick a random point somewhere above the face
            glm::vec3 rayStart = cubeCenter +
                (cubeSide + rayOffset) * faceNormal +
                (cubeSide * (randFloat() - 0.5f)) * secondNormal +
                (cubeSide * (randFloat() - 0.5f)) * thirdNormal;

            // cast multiple rays toward the face
            for (int j = 0; j < numRayCasts; ++j) {
                // pick a random point on the face
                glm::vec3 facePoint = cubeCenter +
                    0.5f * cubeSide * faceNormal +
                    (cubeSide * (randFloat() - 0.5f)) * secondNormal +
                    (cubeSide * (randFloat() - 0.5f)) * thirdNormal;

                // construct a ray from first point through second point
                RayIntersectionInfo intersection;
                intersection._rayStart = rayStart;
                intersection._rayDirection = glm::normalize(facePoint - rayStart);
                intersection._rayLength = 1.0001f * glm::distance(rayStart, facePoint);

                // cast the ray
//                bool hit = cube.findRayIntersection(intersection);

                // validate
                QCOMPARE(cube.findRayIntersection(intersection), true);
//                if (!hit) {
//                    std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should hit cube face" << std::endl;
//                    break;
//                }
                QFUZZY_COMPARE(glm::dot(faceNormal, intersection._hitNormal), 1.0f, EPSILON);
//                if (glm::abs(1.0f - glm::dot(faceNormal, intersection._hitNormal)) > EPSILON) {
//                    std::cout << __FILE__ << ":" << __LINE__
//                        << " ERROR: ray should hit cube face with normal " << faceNormal
//                        << " but found different normal " << intersection._hitNormal << std::endl;
//                }
                QFUZZY_COMPARE(facePoint, intersection.getIntersectionPoint(), EPSILON);
//                if (glm::distance(facePoint, intersection.getIntersectionPoint()) > EPSILON) {
//                    std::cout << __FILE__ << ":" << __LINE__
//                        << " ERROR: ray should hit cube face at " << facePoint
//                        << " but actually hit at " << intersection.getIntersectionPoint()
//                        << std::endl;
//                }
                QCOMPARE(intersection._hitShape, &cube);
//                if (intersection._hitShape != &cube) {
//                    std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray intersection._hitShape should point at cube"
//                        << std::endl;
//                }
            }
        }
    }
}

void ShapeColliderTests::rayMissesAACube() {
    //glm::vec3 cubeCenter(1.23f, 4.56f, 7.89f);
    //float cubeSide = 2.127f;
    glm::vec3 cubeCenter(0.0f);
    float cubeSide = 2.0f;
    AACubeShape cube(cubeSide, cubeCenter);

    float rayOffset = 3.796f;

    glm::vec3 faceNormals[] = {xAxis, yAxis, zAxis};
    int numDirections = 3;
    int numRayCasts = 5;

    const float SOME_SMALL_NUMBER = 0.0001f;

    { // ray misses cube for being too short
        for (int i = 0; i < numDirections; ++i) {
            for (float sign = -1.0f; sign < 2.0f; sign += 2.0f) {
                glm::vec3 faceNormal = sign * faceNormals[i];
                glm::vec3 secondNormal = faceNormals[(i + 1) % numDirections];
                glm::vec3 thirdNormal = faceNormals[(i + 2) % numDirections];

                // pick a random point somewhere above the face
                glm::vec3 rayStart = cubeCenter +
                    (cubeSide + rayOffset) * faceNormal +
                    (cubeSide * (randFloat() - 0.5f)) * secondNormal +
                    (cubeSide * (randFloat() - 0.5f)) * thirdNormal;

                // cast multiple rays toward the face
                for (int j = 0; j < numRayCasts; ++j) {
                    // pick a random point on the face
                    glm::vec3 facePoint = cubeCenter +
                        0.5f * cubeSide * faceNormal +
                        (cubeSide * (randFloat() - 0.5f)) * secondNormal +
                        (cubeSide * (randFloat() - 0.5f)) * thirdNormal;

                    // construct a ray from first point to almost second point
                    RayIntersectionInfo intersection;
                    intersection._rayStart = rayStart;
                    intersection._rayDirection = glm::normalize(facePoint - rayStart);
                    intersection._rayLength = (1.0f - SOME_SMALL_NUMBER) * glm::distance(rayStart, facePoint);

                    // cast the ray
                    QCOMPARE(cube.findRayIntersection(intersection), false);
//                    if (cube.findRayIntersection(intersection)) {
//                        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should NOT hit cube face "
//                            << faceNormal << std::endl;
//                    }
                }
            }
        }
    }
    { // long ray misses cube
        for (int i = 0; i < numDirections; ++i) {
            for (float sign = -1.0f; sign < 2.0f; sign += 2.0f) {
                glm::vec3 faceNormal = sign * faceNormals[i];
                glm::vec3 secondNormal = faceNormals[(i + 1) % numDirections];
                glm::vec3 thirdNormal = faceNormals[(i + 2) % numDirections];

                // pick a random point somewhere above the face
                glm::vec3 rayStart = cubeCenter +
                    (cubeSide + rayOffset) * faceNormal +
                    (cubeSide * (randFloat() - 0.5f)) * secondNormal +
                    (cubeSide * (randFloat() - 0.5f)) * thirdNormal;

                // cast multiple rays that miss the face
                for (int j = 0; j < numRayCasts; ++j) {
                    // pick a random point just outside of face
                    float inside = (cubeSide * (randFloat() - 0.5f));
                    float outside = 0.5f * cubeSide + SOME_SMALL_NUMBER * randFloat();
                    if (randFloat() - 0.5f < 0.0f) {
                        outside *= -1.0f;
                    }
                    glm::vec3 sidePoint = cubeCenter + 0.5f * cubeSide * faceNormal;
                    if (randFloat() - 0.5f < 0.0f) {
                        sidePoint += outside * secondNormal + inside * thirdNormal;
                    } else {
                        sidePoint += inside * secondNormal + outside * thirdNormal;
                    }

                    // construct a ray from first point through second point
                    RayIntersectionInfo intersection;
                    intersection._rayStart = rayStart;
                    intersection._rayDirection = glm::normalize(sidePoint - rayStart);

                    // cast the ray
                    QCOMPARE(cube.findRayIntersection(intersection), false);
//                    if (cube.findRayIntersection(intersection)) {
//                        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should NOT hit cube face "
//                            << faceNormal << std::endl;
//                    }
                }
            }
        }
    }
    { // ray parallel to face barely misses cube
        for (int i = 0; i < numDirections; ++i) {
            for (float sign = -1.0f; sign < 2.0f; sign += 2.0f) {
                glm::vec3 faceNormal = sign * faceNormals[i];
                glm::vec3 secondNormal = faceNormals[(i + 1) % numDirections];
                glm::vec3 thirdNormal = faceNormals[(i + 2) % numDirections];

                // cast multiple rays that miss the face
                for (int j = 0; j < numRayCasts; ++j) {
                    // rayStart is above the face
                    glm::vec3 rayStart = cubeCenter + (0.5f + SOME_SMALL_NUMBER) * cubeSide * faceNormal;

                    // move rayStart to some random edge and choose the ray direction to point across the face
                    float inside = (cubeSide * (randFloat() - 0.5f));
                    float outside = 0.5f * cubeSide + SOME_SMALL_NUMBER * randFloat();
                    if (randFloat() - 0.5f < 0.0f) {
                        outside *= -1.0f;
                    }
                    glm::vec3 rayDirection = secondNormal;
                    if (randFloat() - 0.5f < 0.0f) {
                        rayStart += outside * secondNormal + inside * thirdNormal;
                    } else {
                        rayStart += inside * secondNormal + outside * thirdNormal;
                        rayDirection = thirdNormal;
                    }
                    if (outside > 0.0f) {
                        rayDirection *= -1.0f;
                    }

                    // construct a ray from first point through second point
                    RayIntersectionInfo intersection;
                    intersection._rayStart = rayStart;
                    intersection._rayDirection = rayDirection;

                    // cast the ray
                    QCOMPARE(cube.findRayIntersection(intersection), false);
//                    if (cube.findRayIntersection(intersection)) {
//                        std::cout << __FILE__ << ":" << __LINE__ << " ERROR: ray should NOT hit cube face "
//                                  << faceNormal << std::endl;
//                    }
                }
            }
        }
    }

}

void ShapeColliderTests::measureTimeOfCollisionDispatch() {
    
    // use QBENCHMARK for this
    
    /* KEEP for future manual testing
    // create two non-colliding spheres
    float radiusA = 7.0f;
    float radiusB = 3.0f;
    float alpha = 1.2f;
    float beta = 1.3f;
    glm::vec3 offsetDirection = glm::normalize(glm::vec3(1.0f, 2.0f, 3.0f));
    float offsetDistance = alpha * radiusA + beta * radiusB;

    SphereShape sphereA(radiusA, origin);
    SphereShape sphereB(radiusB, offsetDistance * offsetDirection);
    CollisionList collisions(16);

    //int numTests = 1;
    quint64 oldTime;
    quint64 newTime;
    int numTests = 100000000;
    {
        quint64 startTime = usecTimestampNow();
        for (int i = 0; i < numTests; ++i) {
            ShapeCollider::collideShapes(&sphereA, &sphereB, collisions);
        }
        quint64 endTime = usecTimestampNow();
        std::cout << numTests << " non-colliding collisions in " << (endTime - startTime) << " usec" << std::endl;
        newTime = endTime - startTime;
    }
    */
}

