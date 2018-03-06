
#include "Angel.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

typedef Angel::vec4 point4;
typedef Angel::vec4 color4;
typedef Angel::vec2 vec2;

const int NumVertices = 36; //(6 faces)(2 triangles/face)(3 vertices/triangle)

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

// User parameters to move arm
int old_x, old_y, old_z, new_x, new_y, new_z;
int animation_speed;

// Shader transformation matrices
mat4  model_view;
GLuint ModelView, Projection;

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
colorcube()
{
    quad( 1, 0, 3, 2 );
    quad( 2, 3, 7, 6 );
    quad( 3, 0, 4, 7 );
    quad( 6, 5, 1, 2 );
    quad( 4, 5, 6, 7 );
    quad( 5, 4, 0, 1 );
}

//----------------------------------------------------------------------------

/* Define the three parts */
/* Note use of push/pop to return modelview matrix
to its state before functions were entered and use
rotation, translation, and scaling to create instances
of symbols (cube and cylinder) */

float center_mod = 0.5;

void
base()
{
    mat4 instance = ( Translate( 0.0, 0.5 * BASE_HEIGHT, 0.0 ) *
		              Scale( BASE_WIDTH,
			                 BASE_HEIGHT,
			                 BASE_WIDTH ) );

    glUniformMatrix4fv( ModelView, 1, GL_TRUE, model_view * instance );
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
    glDrawArrays( GL_TRIANGLES, 0, NumVertices );
}

void
sphere()
{
    mat4 instance = ( Translate( 0.0, 0.0, 0.0 ) *
                      Scale( UPPER_ARM_WIDTH,
			                 UPPER_ARM_WIDTH,
			                 UPPER_ARM_WIDTH ) );
    
    glUniformMatrix4fv( ModelView, 1, GL_TRUE, model_view * instance );
    glDrawArrays( GL_TRIANGLES, 0, NumVertices );
}

//----------------------------------------------------------------------------

void
display( void )
{
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    // Accumulate ModelView Matrix as we traverse the tree
    model_view = RotateY( Theta[Base] );
    base();

    model_view *= ( Translate(0.0, BASE_HEIGHT, 0.0) *
		            RotateZ( Theta[LowerArm]) );
    lower_arm();

    // Vector math attempt ******
    //
    // vec2 ball = (old_x, old_y);
    // vec2 bot = (0, LOWER_ARM_HEIGHT + BASE_HEIGHT);
    // vec2 top = (0, LOWER_ARM_HEIGHT + BASE_HEIGHT + UPPER_ARM_HEIGHT);
    // vec2 u = (0.00, UPPER_ARM_HEIGHT);
    // vec2 v = (old_x, (old_y - LOWER_ARM_HEIGHT + BASE_HEIGHT));
    // GLfloat u_length = length(u);
    // GLfloat v_length = length(v);
    // GLfloat num = dot(u, v);
    // GLfloat den = u_length*v_length;
    // GLfloat val = num/den;
    // float angle = acos(val);
    //
    // printf("passing to acos:%f\n", val);
    // printf("u:%f,%f v:%i,%i ul:%f vl:%f num:%f den:%f angle:%f\n", u.x, u.y, v.x, v.y, u_length, v_length, num, den, angle);
    //
    // Theta[UpperArm] = angle;
    
    // Float value attempt *******
    //
    // float adj = abs (old_y - (LOWER_ARM_HEIGHT + BASE_HEIGHT));
    // float hyp = sqrt( old_x*old_x + adj*adj );
    // float angle = cos( adj / hyp ) * (180.0/3.141592653589793238463);
    //
    // if ((old_x * -1) > old_x){ // x is negative
    //     Theta[UpperArm] += 270.0;
    // }
    // if ((old_y * -1) > old_y ){ // y is negative
    //     Theta[UpperArm] -= 90.0;
    // }
    //
    // Theta[UpperArm] = angle;

    Theta[UpperArm] = 90;
    model_view *= ( Translate(old_x - UPPER_ARM_HEIGHT, LOWER_ARM_HEIGHT, 0.0) *
		            RotateZ( Theta[UpperArm]) );
    upper_arm();

    model_view *= ( Translate(old_x, old_y, old_z) *
                    RotateZ( -1 * Theta[UpperArm]) );
    sphere();

    glutSwapBuffers();
}

//----------------------------------------------------------------------------

void
init( void )
{
    colorcube();
    
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

void
menu( int option )
{
    if ( option == Quit ) {
	exit( EXIT_SUCCESS );
    }
    else {
        printf("%i\n",option);
	Axis = option;
    }
}

//----------------------------------------------------------------------------

void
reshape( int width, int height )
{
    glViewport( 0, 0, width, height );

    GLfloat  left = -10.0, right = 10.0;
    GLfloat  bottom = -5.0, top = 15.0;
    GLfloat  zNear = -100.0, zFar = 100.0;

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
    //mat4 projection = Perspective( 90, aspect, zNear, zFar);
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
                // logic for top view
            }
        }
        if (argc >= 7){
            // printf("Enough datapoints to create sphere at (%s, %s, %s) and move to (%s, %s, %s)}\n",
            //                                 argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]);
            old_x = atof(argv[1]);
            old_y = atof(argv[2]);
            old_z = atof(argv[3]);
            new_x = atof(argv[4]);
            new_y = atof(argv[5]);
            new_z = atof(argv[6]);
        }
    }
    
    glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH );
    glutInitWindowSize( 800, 800 );
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

    glutCreateMenu( menu );
    // Set the menu values to the relevant rotation axis values (or Quit)
    glutAddMenuEntry( "base", Base );
    glutAddMenuEntry( "lower arm", LowerArm );
    glutAddMenuEntry( "upper arm", UpperArm );
    glutAddMenuEntry( "quit", Quit );
    glutAttachMenu( GLUT_MIDDLE_BUTTON );


    glutMainLoop();
    return 0;
}
