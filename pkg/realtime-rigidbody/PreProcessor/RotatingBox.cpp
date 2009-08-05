/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "RotatingBox.hpp"
#include<yade/pkg-realtime-rigidbody/FrictionLessElasticContactLaw.hpp>

#include<yade/pkg-common/Box.hpp>
#include<yade/pkg-common/AABB.hpp>
#include<yade/pkg-common/Sphere.hpp>
#include<yade/core/MetaBody.hpp>
#include<yade/core/Body.hpp>
#include<yade/pkg-common/InsertionSortCollider.hpp>
#include<yade/pkg-common/RigidBodyParameters.hpp>
#include<yade/pkg-common/RotationEngine.hpp>

#include<yade/lib-serialization/IOFormatManager.hpp>
#include<yade/pkg-common/InteractingBox.hpp>
#include<yade/pkg-common/InteractingSphere.hpp>
#include<yade/pkg-common/InteractionGeometryMetaEngine.hpp>
#include<yade/pkg-common/CundallNonViscousDamping.hpp>
#include<yade/pkg-common/CundallNonViscousDamping.hpp>
#include<yade/pkg-common/PhysicalActionContainerReseter.hpp>

#include<yade/pkg-common/BoundingVolumeMetaEngine.hpp>
#include<yade/pkg-common/MetaInteractingGeometry2AABB.hpp>
#include<yade/pkg-common/MetaInteractingGeometry.hpp>
#include<yade/pkg-common/GravityEngines.hpp>
#include<yade/pkg-common/PhysicalParametersMetaEngine.hpp>

#include<yade/core/BodyRedirectionVector.hpp>

#include<yade/pkg-common/PhysicalActionDamper.hpp>
#include<yade/pkg-common/PhysicalActionApplier.hpp>


RotatingBox::RotatingBox () : FileGenerator()
{
	nbSpheres	= Vector3r(3,3,5);
	nbBoxes		= Vector3r(3,3,4);
	minSize		= 3;
	maxSize		= 5;
	disorder	= 1.1;
	densityBox	= 1;
	densitySphere	= 1;
	dampingForce	= 0.2;
	dampingMomentum = 0.6;
	isRotating	= true;
	rotationSpeed	= 0.05;
	rotationAxis	= Vector3r(1,1,1);
	middleWireFrame = true;
	gravity 	= Vector3r(0,-9.81,0);
}


RotatingBox::~RotatingBox ()
{
}




bool RotatingBox::generate()
{
	rootBody = shared_ptr<MetaBody>(new MetaBody);

	createActors(rootBody);
	positionRootBody(rootBody);
	

	
	shared_ptr<Body> body;
	
	createKinematicBox(body, Vector3r(  0,  0, 10), Vector3r( 50,  5, 40),middleWireFrame);	rootBody->bodies->insert(body);
	createKinematicBox(body, Vector3r(-55,  0,  0), Vector3r(  5, 60, 50),true );	rootBody->bodies->insert(body);
	createKinematicBox(body, Vector3r( 55,  0,  0), Vector3r(  5, 60, 50),true );	rootBody->bodies->insert(body);
	createKinematicBox(body, Vector3r(  0,-55,  0), Vector3r( 50,  5, 50),true );	rootBody->bodies->insert(body);
	createKinematicBox(body, Vector3r(  0, 55,  0), Vector3r( 50,  5, 50),true );	rootBody->bodies->insert(body);
	createKinematicBox(body, Vector3r(  0,  0,-55), Vector3r( 60, 60,  5),true );	rootBody->bodies->insert(body);
	createKinematicBox(body, Vector3r(  0,  0, 55), Vector3r( 60, 60,  5),true );	rootBody->bodies->insert(body);

	for(int i=0;i<nbSpheres[0];i++)
		for(int j=0;j<nbSpheres[1];j++)
			for(int k=0;k<nbSpheres[2];k++)
			{
				shared_ptr<Body> sphere;
				createSphere(sphere,i,j,k);
				rootBody->bodies->insert(sphere);
			}

	for(int i=0;i<nbBoxes[0];i++)
		for(int j=0;j<nbBoxes[1];j++)
			for(int k=0;k<nbBoxes[2];k++)
 			{
				shared_ptr<Body> box;
				createBox(box,i,j,k);
				rootBody->bodies->insert(box);
 			}
	
	rootBody->dt=1e-2;
	return true;
}



void RotatingBox::createBox(shared_ptr<Body>& body, int i, int j, int k)
{
	body = shared_ptr<Body>(new Body(body_id_t(0),0));
	shared_ptr<RigidBodyParameters> physics(new RigidBodyParameters);
	shared_ptr<AABB> aabb(new AABB);
	shared_ptr<Box> gBox(new Box);
	shared_ptr<InteractingBox> iBox(new InteractingBox);
	
	Quaternionr q;
	q.FromAxisAngle( Vector3r(0,0,1),0);
		
	Vector3r position		= Vector3r(i,j,k)*10
					  - Vector3r(15,35,25)
					  + Vector3r(Mathr::SymmetricRandom(),Mathr::SymmetricRandom(),Mathr::SymmetricRandom());
				  
	Vector3r size 			= Vector3r(     (Mathr::IntervalRandom(minSize,maxSize))
							,(Mathr::IntervalRandom(minSize,maxSize))
							,(Mathr::IntervalRandom(minSize,maxSize))
						);
	body->isDynamic			= true;
	
	physics->angularVelocity	= Vector3r(0,0,0);
	physics->velocity		= Vector3r(0,0,0);
	physics->mass			= size[0]*size[1]*size[2]*densityBox;
	physics->inertia		= Vector3r(
							  physics->mass*(size[1]*size[1]+size[2]*size[2])/3
							, physics->mass*(size[0]*size[0]+size[2]*size[2])/3
							, physics->mass*(size[1]*size[1]+size[0]*size[0])/3
						);
	physics->se3			= Se3r(position,q);

	aabb->diffuseColor		= Vector3r(0,1,0);
	
	gBox->extents			= size;
	gBox->diffuseColor		= Vector3r(Mathr::UnitRandom(),Mathr::UnitRandom(),Mathr::UnitRandom());
	gBox->wire			= false;
	gBox->shadowCaster		= true;
	
	iBox->extents			= size;
	iBox->diffuseColor		= Vector3r(Mathr::UnitRandom(),Mathr::UnitRandom(),Mathr::UnitRandom());

	body->interactingGeometry	= iBox;
	body->geometricalModel		= gBox;
	body->boundingVolume		= aabb;
	body->physicalParameters	= physics;
}


void RotatingBox::createSphere(shared_ptr<Body>& body, int i, int j, int k)
{
	body = shared_ptr<Body>(new Body(body_id_t(0),0));
	shared_ptr<RigidBodyParameters> physics(new RigidBodyParameters);
	shared_ptr<AABB> aabb(new AABB);
	shared_ptr<Sphere> gSphere(new Sphere);
	shared_ptr<InteractingSphere> iSphere(new InteractingSphere);
	
	Quaternionr q;
	q.FromAxisAngle( Vector3r(0,0,1),0);
		
	Vector3r position 		= Vector3r(i,j,k)*10
					  - Vector3r(45,45,45)
					  + Vector3r(Mathr::SymmetricRandom(),Mathr::SymmetricRandom(),Mathr::SymmetricRandom());
				  
	Real radius 			= (Mathr::IntervalRandom(minSize,maxSize));
	
	body->isDynamic			= true;
	
	physics->angularVelocity	= Vector3r(0,0,0);
	physics->velocity		= Vector3r(0,0,0);
	physics->mass			= 4.0/3.0*Mathr::PI*radius*radius*radius*densitySphere;
	physics->inertia		= Vector3r(2.0/5.0*physics->mass*radius*radius,2.0/5.0*physics->mass*radius*radius,2.0/5.0*physics->mass*radius*radius); //
	physics->se3			= Se3r(position,q);

	aabb->diffuseColor		= Vector3r(0,1,0);
	
	gSphere->radius			= radius;
	gSphere->diffuseColor		= Vector3r(Mathr::UnitRandom(),Mathr::UnitRandom(),Mathr::UnitRandom());
	gSphere->wire			= false;
	gSphere->shadowCaster		= true;
	
	iSphere->radius			= radius;
	iSphere->diffuseColor		= Vector3r(Mathr::UnitRandom(),Mathr::UnitRandom(),Mathr::UnitRandom());

	body->interactingGeometry	= iSphere;
	body->geometricalModel		= gSphere;
	body->boundingVolume		= aabb;
	body->physicalParameters	= physics;
}


void RotatingBox::createKinematicBox(shared_ptr<Body>& body, Vector3r position, Vector3r extents,bool wire)
{
	body = shared_ptr<Body>(new Body(body_id_t(0),0));
	shared_ptr<RigidBodyParameters> physics(new RigidBodyParameters);
	shared_ptr<AABB> aabb(new AABB);
	shared_ptr<Box> gBox(new Box);
	shared_ptr<InteractingBox> iBox(new InteractingBox);
	
	Quaternionr q;
	q.FromAxisAngle( Vector3r(0,0,1),0);

	body->isDynamic			= false;
	
	physics->angularVelocity	= Vector3r(0,0,0);
	physics->velocity		= Vector3r(0,0,0);
	physics->mass			= 0;
	physics->inertia		= Vector3r(0,0,0);
	physics->se3			= Se3r(position,q);

	aabb->diffuseColor		= Vector3r(1,0,0);

	gBox->extents			= extents;
	gBox->diffuseColor		= Vector3r(1,1,1);
	gBox->wire			= wire;
	gBox->shadowCaster		= false;
	
	iBox->extents			= extents;
	iBox->diffuseColor		= Vector3r(1,1,1);

	body->boundingVolume		= aabb;
	body->interactingGeometry	= iBox;
	body->geometricalModel		= gBox;
	body->physicalParameters	= physics;
}


void RotatingBox::createActors(shared_ptr<MetaBody>& rootBody)
{
	
	shared_ptr<InteractionGeometryMetaEngine> interactionGeometryDispatcher(new InteractionGeometryMetaEngine);
	interactionGeometryDispatcher->add("InteractingSphere2InteractingSphere4ClosestFeatures");
	interactionGeometryDispatcher->add("InteractingBox2InteractingSphere4ClosestFeatures");
	interactionGeometryDispatcher->add("InteractingBox2InteractingBox4ClosestFeatures");

	shared_ptr<BoundingVolumeMetaEngine> boundingVolumeDispatcher	= shared_ptr<BoundingVolumeMetaEngine>(new BoundingVolumeMetaEngine);
	boundingVolumeDispatcher->add("InteractingSphere2AABB");
	boundingVolumeDispatcher->add("InteractingBox2AABB");
	boundingVolumeDispatcher->add("MetaInteractingGeometry2AABB");
		
	shared_ptr<GravityEngine> gravityCondition(new GravityEngine);
	gravityCondition->gravity = gravity;
	
	shared_ptr<CundallNonViscousForceDamping> actionForceDamping(new CundallNonViscousForceDamping);
	actionForceDamping->damping = dampingForce;
	shared_ptr<CundallNonViscousMomentumDamping> actionMomentumDamping(new CundallNonViscousMomentumDamping);
	actionMomentumDamping->damping = dampingMomentum;
	shared_ptr<PhysicalActionDamper> actionDampingDispatcher(new PhysicalActionDamper);
	actionDampingDispatcher->add(actionForceDamping);
	actionDampingDispatcher->add(actionMomentumDamping);
	
	shared_ptr<PhysicalActionApplier> applyActionDispatcher(new PhysicalActionApplier);
	applyActionDispatcher->add("NewtonsForceLaw");
	applyActionDispatcher->add("NewtonsMomentumLaw");
	
	shared_ptr<PhysicalParametersMetaEngine> positionIntegrator(new PhysicalParametersMetaEngine);
	positionIntegrator->add("LeapFrogPositionIntegrator");
	shared_ptr<PhysicalParametersMetaEngine> orientationIntegrator(new PhysicalParametersMetaEngine);
	orientationIntegrator->add("LeapFrogOrientationIntegrator");
 	
	shared_ptr<RotationEngine> kinematic = shared_ptr<RotationEngine>(new RotationEngine);
 	kinematic->angularVelocity  = rotationSpeed;
	rotationAxis.Normalize();
 	kinematic->rotationAxis  = rotationAxis;
 	kinematic->rotateAroundZero = true;
	
 	for(int i=0;i<7;i++)
 		kinematic->subscribedBodies.push_back(i);
	
	rootBody->engines.clear();
	rootBody->engines.push_back(shared_ptr<Engine>(new PhysicalActionContainerReseter));
	rootBody->engines.push_back(boundingVolumeDispatcher);
	rootBody->engines.push_back(shared_ptr<Engine>(new InsertionSortCollider));
	rootBody->engines.push_back(interactionGeometryDispatcher);
	rootBody->engines.push_back(shared_ptr<Engine>(new FrictionLessElasticContactLaw));
	rootBody->engines.push_back(gravityCondition);
	rootBody->engines.push_back(actionDampingDispatcher);
	rootBody->engines.push_back(applyActionDispatcher);
	rootBody->engines.push_back(positionIntegrator);
	rootBody->engines.push_back(orientationIntegrator);
	if(isRotating)
		rootBody->engines.push_back(kinematic);
		
	rootBody->initializers.clear();
	rootBody->initializers.push_back(boundingVolumeDispatcher);
}



void RotatingBox::positionRootBody(shared_ptr<MetaBody>& rootBody)
{
	rootBody->isDynamic			= false;
	Quaternionr q;	q.FromAxisAngle( Vector3r(0,0,1),0);
	shared_ptr<ParticleParameters> physics(new ParticleParameters); // FIXME : fix indexable class PhysicalParameters
	physics->se3				= Se3r(Vector3r(0,0,0),q);
	physics->mass				= 0;
	physics->velocity			= Vector3r::ZERO;
	physics->acceleration			= Vector3r::ZERO;
	
	shared_ptr<MetaInteractingGeometry> set(new MetaInteractingGeometry());
	set->diffuseColor			= Vector3r(0,0,1);

	shared_ptr<AABB> aabb(new AABB);
	aabb->diffuseColor			= Vector3r(0,0,1);
	
	rootBody->interactingGeometry		= YADE_PTR_CAST<InteractingGeometry>(set);	
	rootBody->boundingVolume		= YADE_PTR_CAST<BoundingVolume>(aabb);
	rootBody->physicalParameters 		= physics;
}


YADE_PLUGIN((RotatingBox));
