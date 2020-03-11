//
//  Camera.cpp
//  Lab5
//
//  Created by CGIS on 28/10/2016.
//  Copyright © 2016 CGIS. All rights reserved.
//
#include "stdafx.h" 
#include "Camera.hpp"


namespace gps {
    
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget)
    {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraDirection = glm::normalize(cameraTarget - cameraPosition);
        this->cameraRightDirection = glm::normalize(glm::cross(this->cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f)));
    }
    
    glm::mat4 Camera::getViewMatrix()
    {
        return glm::lookAt(cameraPosition, cameraPosition + cameraDirection , glm::vec3(0.0f, 1.0f, 0.0f));
    }

	glm::vec3 Camera::getCameraTarget()
	{
		return cameraTarget;
	}

	void Camera::setCameraTarget(float x, float y, float z)
	{
		cameraTarget = glm::vec3(x, y, z);
	}

	glm::vec3 Camera::getCameraPosition()
	{
		return cameraPosition;
	}

	glm::vec3 Camera::getCameraDirection()
	{
		return cameraDirection;

	}

	void Camera::setCameraDirection(glm::vec3 dir)
	{
		 cameraDirection = dir;
	}

	glm::vec3 Camera::getCameraRightDirection()
	{
		return cameraRightDirection;
	}

	void Camera::setCameraPosition(float x, float y, float z)
	{
		cameraPosition = glm::vec3(x, y, z);
	}
    
    void Camera::move(MOVE_DIRECTION direction, float speed)
    {
        switch (direction) {
            case MOVE_FORWARD:
                cameraPosition += cameraDirection * speed;
                break;
                
            case MOVE_BACKWARD:
                cameraPosition -= cameraDirection * speed;
                break;
                
            case MOVE_RIGHT:
                cameraPosition += cameraRightDirection * speed;
                break;
                
            case MOVE_LEFT:
                cameraPosition -= cameraRightDirection * speed;
                break;
        }
    }
    
    void Camera::rotate(float pitch, float yaw)
    {
		cameraDirection.y = sin(glm::radians(pitch));
		cameraDirection.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		cameraDirection = glm::normalize(cameraDirection);
		cameraUp = glm::normalize(cross(cameraRightDirection, cameraDirection));
		cameraDirection.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		this->cameraRightDirection = glm::normalize(glm::cross(this->cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f)));
    }
    
}
