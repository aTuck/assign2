
#include "Angel.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

typedef Angel::vec4 point4;
typedef Angel::vec4 color4;
typedef Angel::vec2 vec2;

const int NumVertices = 36; //(6 faces)(2 triangles/face)(3 vertices/triangle)
const int NumSphereVertices = 342; // 8 rows of 18 quads

point4 quad_data[NumSphereVertices]; 
point4 points[NumVertices];
color4 colors[NumVertices];

point4 vertices[8] = {
    point4( -0.5, -0.5,  0.5, 1.0 ),
    point4( -0.5,  0.5,  0.5, 1.0 ),
    point4(  0.5,  0.5,  0.5, 1.0 ),
    point4(  0.5, -0.5,  0.5, 1.0 ),
    point4( -0.5, -0.5, -0.5, 1.0 ),
    point4( -0.5,  0.5, -0.5, 1.0 ),
    point4(  0.5,  0.5, -0.5, 1.0 ),
    point4(  0.5, -0.5, -0.5, 1.0 )
};

// RGBA olors
color4 vertex_colors[8] = {
    color4( 0.0, 0.0, 0.0, 1.0 ),  // black
    color4( 1.0, 0.0, 0.0, 1.0 ),  // red
    color4( 1.0, 1.0, 0.0, 1.0 ),  // yellow
    color4( 0.0, 1.0, 0.0, 1.0 ),  // green
    color4( 0.0, 0.0, 1.0, 0.0 ),  // blue
    color4( 1.0, 0.0, 1.0, 1.0 ),  // magenta
    color4( 1.0, 1.0, 1.0, 1.0 ),  // white
    color4( 0.0, 1.0, 1.0, 1.0 )   // cyan
};


// Parameters controlling the size of the Robot's arm
const GLfloat BASE_HEIGHT      = 2.0;
const GLfloat BASE_WIDTH       = 5.0;
const GLfloat LOWER_ARM_HEIGHT = 5.0;
const GLfloat LOWER_ARM_WIDTH  = 0.5;
const GLfloat UPPER_ARM_HEIGHT = 5.0;
const GLfloat UPPER_ARM_WIDTH  = 0.5;
const float RadiansToDegrees = 180/M_PI;

// User parameters to move arm
int   oldCoords [3];
int   newCoords [3];

// Calculated angles
float oldAngles [3];
float newAngles [3];
float iAngles [3] = {0, 0, 0};

// Variables for performance tweaks
float speed = 0.9;
float tol = 0.001;

// Angles
float bendAngle = 0;
float tiltAngle = 0;
float zAngle = 0;
float lengthToSphere = 0;

// Trigger next movement path
bool doneOld [3] = {0, 0, 0};
bool doneBall = 0;
bool calcedAngles = 0;
bool isTopView = 0;

// Shader transformation matrices
mat4  model_view;
GLuint ModelView, Projection;

// vao & buffers
GLuint spherevao = 0;
GLuint cubevao = 0;
GLuint spherebuffer = 0;
GLuint cubebuffer = 0;

// Array of rotation angles (in degrees) for each rotation axis
enum { Base = 0, LowerArm = 1, UpperArm = 2, NumAngles = 3 };
int      Axis = Base;
GLfloat  Theta[NumAngles] = { 0.0 };

// Menu option values
const int  Quit = 4;

//----------------------------------------------------------------------------

int Index = 0;

void
quad( int a, int b, int c, int d )
{
    colors[Index] = vertex_colors[a]; points[Index] = vertices[a]; Index++;
    colors[Index] = vertex_colors[a]; points[Index] = vertices[b]; Index++;
    colors[Index] = vertex_colors[a]; points[Index] = vertices[c]; Index++;
    colors[Index] = vertex_colors[a]; points[Index] = vertices[a]; Index++;
    colors[Index] = vertex_colors[a]; points[Index] = vertices[c]; Index++;
    colors[Index] = vertex_colors[a]; points[Index] = vertices[d]; Index++;
}

void
createCubeBufferAndVao()
{
    // Create a vertex array object
    GLuint vao;
    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao );

    // Create and initialize a buffer object
    GLuint buffer;
    glGenBuffers( 1, &buffer );
    glBindBuffer( GL_ARRAY_BUFFER, buffer );    
    glBufferData( GL_ARRAY_BUFFER, sizeof(points) + sizeof(colors),
		  NULL, GL_DYNAMIC_DRAW );
    glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(points), points );
    glBufferSubData( GL_ARRAY_BUFFER, sizeof(points), sizeof(colors), colors );

    // Load shaders and use the resulting shader program
    GLuint program = InitShader( "vshader81.glsl", "fshader81.glsl" );
    glUseProgram( program );
    
    GLuint vPosition = glGetAttribLocation( program, "vPosition" );
    glEnableVertexAttribArray( vPosition );
    glVertexAttribPointer( vPosition, 4, GL_FLOAT, GL_FALSE, 0,
			   BUFFER_OFFSET(0) );

    GLuint vColor = glGetAttribLocation( program, "vColor" );
    glEnableVertexAttribArray( vColor );
    glVertexAttribPointer( vColor, 4, GL_FLOAT, GL_FALSE, 0,
			   BUFFER_OFFSET(sizeof(points)) );

    ModelView = glGetUniformLocation( program, "ModelView" );
    Projection = glGetUniformLocation( program, "Projection" );

    cubebuffer = buffer;
    cubevao = vao;
}

void
colorcube()
{
    quad( 1, 0, 3, 2 );
    quad( 2, 3, 7, 6 );
    quad( 3, 0, 4, 7 );
    quad( 6, 5, 1, 2 );
    quad( 4, 5, 6, 7 );
    quad( 5, 4, 0, 1 );
    createCubeBufferAndVao();
}

void
createSphereBufferAndVao()
{
    // Create VAO for sphere
    GLuint vao;
    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao );

    // Create and initialize a buffer object for sphere
    GLuint buffer;
    glGenBuffers( 1, &buffer );
    glBindBuffer( GL_ARRAY_BUFFER, buffer );  
    glBufferData( GL_ARRAY_BUFFER, sizeof(quad_data) + sizeof(colors),
		  NULL, GL_DYNAMIC_DRAW );
    glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(quad_data), quad_data );
    glBufferSubData( GL_ARRAY_BUFFER, sizeof(quad_data), sizeof(colors), colors );
    
    // Load shaders and use the resulting shader program
    GLuint program = InitShader( "vshader81.glsl", "fshader81.glsl" );
    glUseProgram( program );

    GLuint vPosition = glGetAttribLocation( program, "vPosition" );
    glEnableVertexAttribArray( vPosition );
    glVertexAttribPointer( vPosition, 4, GL_FLOAT, GL_FALSE, 0,
			   BUFFER_OFFSET(0) );

    GLuint vColor = glGetAttribLocation( program, "vColor" );
    glEnableVertexAttribArray( vColor );
    glVertexAttribPointer( vColor, 4, GL_FLOAT, GL_FALSE, 0,
			   BUFFER_OFFSET(sizeof(quad_data)) );
            
    ModelView = glGetUniformLocation( program, "ModelView" );
    Projection = glGetUniformLocation( program, "Projection" );

    spherebuffer = buffer;
    spherevao = vao;
}

void
sphere()
{
    const float DegreesToRadians = M_PI / 180.0; // M_PI = 3.14159...
   
    int k = 0;
    for(float phi = -80.0; phi <= 80.0; phi += 20.0)
    {
        float phir = phi*DegreesToRadians;
        float phir20 = (phi + 20.0)*DegreesToRadians;
        for(float theta = -180.0; theta <= 180.0; theta += 20.0)
        {
            float thetar = theta*DegreesToRadians;

            quad_data[k] = point4(sin(thetar)*cos(phir),
                                  cos(thetar)*cos(phir), sin(phir), 1);
            k++;

            quad_data[k] = point4(sin(thetar)*cos(phir20),
                                  cos(thetar)*cos(phir20), sin(phir20), 1);
            k++;
        }
    }
    createSphereBufferAndVao();
}


//----------------------------------------------------------------------------

/* Define the three parts */
/* Note use of push/pop to return modelview matrix
to its state before functions were entered and use
rotation, translation, and scaling to create instances
of symbols (cube and cylinder) */
void
base()
{
    mat4 instance = ( Translate( 0.0, 0.5 * BASE_HEIGHT, 0.0 ) *
		              Scale( BASE_WIDTH,
			                 BASE_HEIGHT,
			                 BASE_WIDTH ) );

    glUniformMatrix4fv( ModelView, 1, GL_TRUE, model_view * instance );
    glBindVertexArray( cubevao );
    glBindBuffer ( GL_ARRAY_BUFFER, cubebuffer  );
    glDrawArrays( GL_TRIANGLES, 0, NumVertices );
}

void
lower_arm()
{
    mat4 instance = ( Translate( 0.0, 0.5 * LOWER_ARM_HEIGHT, 0.0 ) *
		              Scale( LOWER_ARM_WIDTH,
			                 LOWER_ARM_HEIGHT,
			                 LOWER_ARM_WIDTH ) );
    
    glUniformMatrix4fv( ModelView, 1, GL_TRUE, model_view * instance );
    glBindVertexArray( cubevao );
    glBindBuffer ( GL_ARRAY_BUFFER, cubebuffer );
    glDrawArrays( GL_TRIANGLES, 0, NumVertices );
}

void
upper_arm()
{
    mat4 instance = ( Translate( 0.0, 0.5 * UPPER_ARM_HEIGHT, 0.0 ) *
		              Scale( UPPER_ARM_WIDTH,
			                 UPPER_ARM_HEIGHT,
                             UPPER_ARM_WIDTH ) );
    
    glUniformMatrix4fv( ModelView, 1, GL_TRUE, model_view * instance );
    glBindVertexArray( cubevao );
    glBindBuffer ( GL_ARRAY_BUFFER, cubebuffer );
    glDrawArrays( GL_TRIANGLES, 0, NumVertices );
}

void
ball()
{
    mat4 instance = ( Scale( UPPER_ARM_WIDTH,
			                 UPPER_ARM_WIDTH,
			                 UPPER_ARM_WIDTH ) );
    
    glUniformMatrix4fv( ModelView, 1, GL_TRUE, model_view * instance );
    glBindVertexArray( spherevao );
    glBindBuffer ( GL_ARRAY_BUFFER, spherebuffer );
    glDrawArrays( GL_TRIANGLE_FAN, 0, NumSphereVertices );
}

//----------------------------------------------------------------------------

int
getDir(int x, int y){
    // Is destination angle more than current angle? 
    // If so, want to increase angle, otherwise decrease angle
    return (x - y) < 0 ? 1 : -1;
}

bool
calcAngles(){
    float ix, iy, iz = 0;
    
    // oldAngles
    ix = oldCoords[0]; // x
    iy = oldCoords[1]; // y
    iz = oldCoords[2]; // z
    lengthToSphere = sqrt(ix*ix + iy*iy + iz*iz);
    bendAngle = cos(lengthToSphere/10)*cos(lengthToSphere/10) - (sin(lengthToSphere/10) * sin(lengthToSphere/10));
    bendAngle *= RadiansToDegrees;
    tiltAngle = acos( iy / sqrt(iz*iz + ix*ix + iy*iy) );
    tiltAngle *= RadiansToDegrees;
    zAngle = atan( iz / sqrt(iz*iz+ix*ix) );
    zAngle *= RadiansToDegrees;
    if (0 - ix < 0){ //negative
        zAngle += 180; //mirror over x axis
    }
    oldAngles[0] = bendAngle;
    oldAngles[1] = tiltAngle;
    oldAngles[2] = zAngle;

    // newAngles
    ix = newCoords[0]; // x
    iy = newCoords[1]; // y
    iz = newCoords[2]; // z
    lengthToSphere = sqrt(ix*ix + iy*iy + iz*iz);
    bendAngle = cos(lengthToSphere/10)*cos(lengthToSphere/10) - (sin(lengthToSphere/10) * sin(lengthToSphere/10));
    bendAngle *= RadiansToDegrees;
    tiltAngle = acos( iy / sqrt(iz*iz + ix*ix + iy*iy) );
    tiltAngle *= RadiansToDegrees;
    zAngle = atan( iz / sqrt(iz*iz+ix*ix) );
    zAngle *= RadiansToDegrees;
    if (0 - ix < 0){ //negative
        zAngle += 180; // mirror over x axis
    }
    newAngles[0] = bendAngle;
    newAngles[1] = tiltAngle;
    newAngles[2] = zAngle;

    // Print all the angles
    // printf("OLD: lengthToSphere is:%f\n", lengthToSphere);
    printf("OLD: bendAngle is:%f\n", oldAngles[0]);
    printf("OLD: tiltAngle is:%f\n", oldAngles[1]);
    printf("OLD: zAngle is:%f\n\n", oldAngles[2]);

    // printf("NEW: lengthToSphere is:%f\n", lengthToSphere);
    printf("NEW: bendAngle is:%f\n", newAngles[0]);
    printf("NEW: tiltAngle is:%f\n", newAngles[1]);
    printf("NEW: zAngle is:%f\n\n", newAngles[2]);

    return 1;
}

bool
moveToOld(int i){
    int dir =  getDir(iAngles[i], oldAngles[i]);
    if (abs(iAngles[i] - oldAngles[i]) > tol && !doneOld[i]){
        iAngles[i] += (speed*dir);
        printf("Angle(%i) toOld:%f\n", i, iAngles[i]);
        return 1;
    }
    else{
        doneOld[i] = 1;
        return 0;
    }
}

bool
moveToNew(int i){
    int dir =  getDir(oldAngles[i], newAngles[i]);
    if(abs(iAngles[i] - newAngles[i]) > tol){
        iAngles[i] += (speed*dir);
        printf("Angle(%i) toNew:%f\n", i, iAngles[i]);
        return 1;
    }
    else{
        return 0;
    }
}

bool 
moveToDefault(int i){
    int dir = getDir(newAngles[i], 0);
    if(abs(iAngles[i] - 0) > tol){
        iAngles[i] += (speed*dir);
        printf("Angle(%i) toDefault:%f\n", i, iAngles[i]);
        return 1;
    }
    else{
        return 0;
    } 
}

mat4 
initTopView(){
    point4 at(0.0, 0.0, 3.5, 1.0); // origin and panned
    point4 eye(0.0, 2.0, 3.5, 1.0); // eye above and panned
    vec4   up(0.0, 0.0, -1.0, 0.0); // -z is up
    return LookAt(eye, at, up);
}

void
display( void )
{
    // bindColorCube();
    
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    if (!calcedAngles){
        calcAngles();
        calcedAngles = 1;
    }

    // old to new
    if ( !moveToOld(0) && !moveToOld(1) && !moveToOld(2) && !doneBall ){
        if ( !moveToNew(0) && !moveToNew(1) && !moveToNew(2) ){
            doneBall = 1;
        }
    }

    if (doneBall){
        if (!moveToDefault(0) && !moveToDefault(1) && !moveToDefault(2)){
            iAngles[0] = 0;
            iAngles[1] = 0;
            iAngles[2] = 0;
            printf("Done!\n");
        }
    }

    if (isTopView){ model_view = initTopView(); }
    else          { model_view = mat4( 1.0 );   }

    // Accumulate ModelView Matrix as we traverse the tree
    model_view *= RotateY( iAngles[2] );
    base();

    // Lower Arm
    model_view *= ( Translate(0.0, BASE_HEIGHT, 0.0) *
                    RotateZ( -1 * iAngles[0] ) * RotateZ( iAngles[1] ) );
    lower_arm();

    // Upper Arm
    model_view *= ( Translate(0.0, LOWER_ARM_HEIGHT, 0.0) *
                    RotateZ( 2 * iAngles[0] ) );
    upper_arm();

    // bindSphere();
    // Old sphere
    if ( !(doneOld[0] && doneOld[1] && doneOld[2]) ){
        if (isTopView){ model_view = initTopView(); }
        else          { model_view = mat4( 1.0 );   }
        model_view *= ( Translate( oldCoords[0], oldCoords[1], oldCoords[2] ) );
        ball();
    }
    // Sphere on arm
    else if ((doneOld[0] && doneOld[1] && doneOld[2]) && !doneBall ){
        model_view *= ( Translate(0.0, UPPER_ARM_HEIGHT + 0.35, 0.0) );
        ball();
    }
    // New sphere
    if (doneBall){
        if (isTopView){ model_view = initTopView(); }
        else          { model_view = mat4( 1.0 );   }
        model_view = ( Translate( newCoords[0], newCoords[1], newCoords[2] ) );
        ball();
    }
    
    glutPostRedisplay();
    glutSwapBuffers();
}

//----------------------------------------------------------------------------

void
init( void )
{
    colorcube();
    sphere();

    glEnable( GL_DEPTH );
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

    glClearColor( 0.05, 0.05, 0.05, 0.05 ); 
}

//----------------------------------------------------------------------------

void
mouse( int button, int state, int x, int y )
{

    if ( button == GLUT_LEFT_BUTTON  && state == GLUT_DOWN ) {
	// Incrase the joint angle
	Theta[Axis] += 5.0;
	if ( Theta[Axis] > 360.0 ) { Theta[Axis] -= 360.0; }
    }

    if ( button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN ) {
	// Decrase the joint angle
	Theta[Axis] -= 5.0;
	if ( Theta[Axis] < 0.0 ) { Theta[Axis] += 360.0; }
    }

    glutPostRedisplay();
}

//----------------------------------------------------------------------------

void
reshape( int width, int height )
{
    glViewport( 0, 0, width, height );

    GLfloat  left = -10.0, right = 10.0;
    GLfloat  bottom = -5.0, top = 15.0;
    GLfloat  zNear = -10.0, zFar = 100.0;

    GLfloat aspect = GLfloat(width)/height;

    if ( aspect > 1.0 ) {
	    left *= aspect;
	    right *= aspect;
    }
    else {
	    bottom /= aspect;
	    top /= aspect;
    }

    mat4 projection = Ortho( left, right, bottom, top, zNear, zFar );
    glUniformMatrix4fv( Projection, 1, GL_TRUE, projection );

    model_view = mat4( 1.0 );  // An Identity matrix
}

//----------------------------------------------------------------------------

void
keyboard( unsigned char key, int x, int y )
{
    switch( key ) {
	case 033: // Escape Key
	case 'q': case 'Q':
	    exit( EXIT_SUCCESS );
	    break;
    }
}

//----------------------------------------------------------------------------
int
main( int argc, char **argv )
{
    glutInit( &argc, argv );

    if(argc > 0){
        for(int i = 1; i < argc; i++){
            if (!strcmp(argv[i], "-tv")){
                printf("Setting logic for top view.\n");
                isTopView = 1;
            }
        }
        if (argc >= 7){
            // printf("Enough datapoints to create sphere at (%s, %s, %s) and move to (%s, %s, %s)}\n",
            //                                 argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]);
            oldCoords[0] = atof(argv[1]);
            oldCoords[1] = atof(argv[2]);
            oldCoords[2] = atof(argv[3]);
            newCoords[0] = atof(argv[4]);
            newCoords[1] = atof(argv[5]);
            newCoords[2] = atof(argv[6]);
            for (int i = 0; i < 3; i++){
                if ( abs(oldCoords[i]) > 9 || oldCoords[i] < -4 ) {
                    printf("%i is unreachable, please enter values between -4 and 9 inclusive.\n", oldCoords[i]);
                    return 0;
                }
                else if ( abs(newCoords[i]) > 9 || newCoords[i] < -4 ){
                    printf("%i is unreachable, please enter values between -4 and 9 inclusive.\n", newCoords[i]);
                    return 0;
                }
            }
        }
    }

    glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH );
    glutInitWindowSize( 1200, 1200 );
    glutInitContextVersion( 3, 2 );
    glutInitContextProfile( GLUT_CORE_PROFILE );
    glutCreateWindow( "robot" );

    // If you get a segmentation error at line 34, please uncomment the line below
    glewExperimental = GL_TRUE; 
    glewInit();
    
    init();

    glutDisplayFunc( display );
    glutReshapeFunc( reshape );
    glutKeyboardFunc( keyboard );
    glutMouseFunc( mouse );

    glutMainLoop();
    return 0;
}
