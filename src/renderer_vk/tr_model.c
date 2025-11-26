/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).  

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// tr_models.c -- model loading and caching

#include "tr_local.h"


#define LL( x ) x = LittleLong( x )

// Ridah
static qboolean R_LoadMDC( model_t *mod, int lod, void *buffer, const char *mod_name );
// done.
static qboolean R_LoadMD3( model_t *mod, int lod, void *buffer, const char *name );
static qboolean R_LoadMDS( model_t *mod, void *buffer, const char *name );

model_t *loadmodel;

extern cvar_t *r_exportCompressedModels;
extern cvar_t *r_buildScript;

/*
** R_GetModelByHandle
*/
model_t *R_GetModelByHandle( qhandle_t index ) {
	model_t     *mod;

	// out of range gets the defualt model
	if ( index < 1 || index >= tr.numModels ) {
		return tr.models[0];
	}

	mod = tr.models[index];

	return mod;
}

//===============================================================================

/*
** R_AllocModel
*/
model_t *R_AllocModel( void ) {
	model_t     *mod;

	if ( tr.numModels == MAX_MOD_KNOWN ) {
		return NULL;
	}

	mod = ri.Hunk_Alloc( sizeof( *tr.models[tr.numModels] ), h_low );
	mod->index = tr.numModels;
	tr.models[tr.numModels] = mod;
	tr.numModels++;

	return mod;
}

/*
====================
RE_RegisterModel

Loads in a model for the given name

Zero will be returned if the model fails to load.
An entry will be retained for failed models as an
optimization to prevent disk rescanning if they are
asked for again.
====================
*/
qhandle_t RE_RegisterModel( const char *name ) {
	model_t     *mod;
	unsigned    *buf;
	int lod;
	int ident = 0;         // TTimo: init
	qboolean loaded;
	qhandle_t hModel;
	int numLoaded;

	if ( !name || !name[0] ) {
		// Ridah, disabled this, we can see models that can't be found because they won't be there
		//ri.Printf( PRINT_ALL, "RE_RegisterModel: NULL name\n" );
		return 0;
	}

	if ( strlen( name ) >= MAX_QPATH ) {
		Com_Printf( "Model name exceeds MAX_QPATH\n" );
		return 0;
	}

	//
	// search the currently loaded models
	//
	for ( hModel = 1 ; hModel < tr.numModels; hModel++ ) {
		mod = tr.models[hModel];
		if ( !Q_stricmp( mod->name, name ) ) {
			if ( mod->type == MOD_BAD ) {
				return 0;
			}
			return hModel;
		}
	}

	// allocate a new model_t

	if ( ( mod = R_AllocModel() ) == NULL ) {
		ri.Printf( PRINT_WARNING, "RE_RegisterModel: R_AllocModel() failed for '%s'\n", name );
		return 0;
	}

	// only set the name after the model has been successfully loaded
	Q_strncpyz( mod->name, name, sizeof( mod->name ) );

	mod->numLods = 0;

	//
	// load the files
	//
	numLoaded = 0;

	if ( strstr( name, ".mds" ) ) {  // try loading skeletal file
		loaded = qfalse;
		ri.FS_ReadFile( name, (void **)&buf );
		if ( buf ) {
			loadmodel = mod;

			ident = LittleLong( *(unsigned *)buf );
			if ( ident == MDS_IDENT ) {
				loaded = R_LoadMDS( mod, buf, name );
			}

			ri.FS_FreeFile( buf );
		}

		if ( loaded ) {
			return mod->index;
		}
	}

	for ( lod = MD3_MAX_LODS - 1 ; lod >= 0 ; lod-- ) {
		char filename[1024];

		strcpy( filename, name );

		if ( lod != 0 ) {
			char namebuf[80];

			if ( strrchr( filename, '.' ) ) {
				*strrchr( filename, '.' ) = 0;
			}
			sprintf( namebuf, "_%d.md3", lod );
			strcat( filename, namebuf );
		}


		filename[strlen( filename ) - 1] = 'c';  // try MDC first

		ri.FS_ReadFile( filename, (void **)&buf );

		if ( !buf ) {
			
			filename[strlen( filename ) - 1] = '3';  // try MD3 second
			
			ri.FS_ReadFile( filename, (void **)&buf );
			if ( !buf ) {
				continue;
			}
		}

		loadmodel = mod;

		ident = LittleLong( *(unsigned *)buf );
		// Ridah, mesh compression
		if ( ident != MD3_IDENT && ident != MDC_IDENT ) {
			ri.Printf( PRINT_WARNING,"RE_RegisterModel: unknown fileid for %s\n", name );
			goto fail;
		}

		if ( ident == MD3_IDENT ) {
			loaded = R_LoadMD3( mod, lod, buf, name );
		} else {
			loaded = R_LoadMDC( mod, lod, buf, name );
		}
		// done.

		ri.FS_FreeFile( buf );

		if ( !loaded ) {
			if ( lod == 0 ) {
				goto fail;
			} else {
				break;
			}
		} else {
			mod->numLods++;
			numLoaded++;
			// if we have a valid model and are biased
			// so that we won't see any higher detail ones,
			// stop loading them
			if ( lod <= r_lodbias->integer ) {
				break;
			}
		}
	}


	if ( numLoaded ) {
		// duplicate into higher lod spots that weren't
		// loaded, in case the user changes r_lodbias on the fly
		for ( lod-- ; lod >= 0 ; lod-- ) {
			mod->numLods++;
			// Ridah, mesh compression
			//	this check for mod->md3[0] could leave mod->md3[0] == 0x0000000 if r_lodbias is set to anything except '0'
			//	which causes trouble in tr_mesh.c in R_AddMD3Surfaces() and other locations since it checks md3[0]
			//	for various things.
			if ( ident == MD3_IDENT ) { //----(SA)	modified
//			if (mod->md3[0])		//----(SA)	end
				mod->md3[lod] = mod->md3[lod + 1];
			} else {
				mod->mdc[lod] = mod->mdc[lod + 1];
			}
			// done.
		}

		return mod->index;
	}

fail:
	// we still keep the model_t around, so if the model name is asked for
	// again, we won't bother scanning the filesystem
	mod->type = MOD_BAD;
	return 0;
}





/*
=================
R_LoadMDC
=================
*/
static qboolean R_LoadMDC( model_t *mod, int lod, void *buffer, const char *mod_name ) {
	int i, j;
	mdcHeader_t         *pinmodel;
	md3Frame_t          *frame;
	mdcSurface_t        *surf;
	md3Shader_t         *shader;
	md3Triangle_t       *tri;
	md3St_t             *st;
	md3XyzNormal_t      *xyz;
	mdcXyzCompressed_t  *xyzComp;
	mdcTag_t            *tag;
	short               *ps;
	int version;
	int size;

	pinmodel = (mdcHeader_t *)buffer;

	version = LittleLong( pinmodel->version );
	if ( version != MDC_VERSION ) {
		ri.Printf( PRINT_WARNING, "R_LoadMDC: %s has wrong version (%i should be %i)\n",
				   mod_name, version, MDC_VERSION );
		return qfalse;
	}

	mod->type = MOD_MDC;
	size = LittleLong( pinmodel->ofsEnd );
	mod->dataSize += size;
	mod->mdc[lod] = ri.Hunk_Alloc( size, h_low );

	memcpy( mod->mdc[lod], buffer, LittleLong( pinmodel->ofsEnd ) );

	LL( mod->mdc[lod]->ident );
	LL( mod->mdc[lod]->version );
	LL( mod->mdc[lod]->numFrames );
	LL( mod->mdc[lod]->numTags );
	LL( mod->mdc[lod]->numSurfaces );
	LL( mod->mdc[lod]->ofsFrames );
	LL( mod->mdc[lod]->ofsTagNames );
	LL( mod->mdc[lod]->ofsTags );
	LL( mod->mdc[lod]->ofsSurfaces );
	LL( mod->mdc[lod]->ofsEnd );
	LL( mod->mdc[lod]->flags );
	LL( mod->mdc[lod]->numSkins );


	if ( mod->mdc[lod]->numFrames < 1 ) {
		ri.Printf( PRINT_WARNING, "R_LoadMDC: %s has no frames\n", mod_name );
		return qfalse;
	}

	// swap all the frames
	frame = ( md3Frame_t * )( (byte *)mod->mdc[lod] + mod->mdc[lod]->ofsFrames );
	for ( i = 0 ; i < mod->mdc[lod]->numFrames ; i++, frame++ ) {
		frame->radius = LittleFloat( frame->radius );
		if ( strstr( mod->name,"sherman" ) || strstr( mod->name, "mg42" ) ) {
			frame->radius = 256;
			for ( j = 0 ; j < 3 ; j++ ) {
				frame->bounds[0][j] = 128;
				frame->bounds[1][j] = -128;
				frame->localOrigin[j] = LittleFloat( frame->localOrigin[j] );
			}
		} else
		{
			for ( j = 0 ; j < 3 ; j++ ) {
				frame->bounds[0][j] = LittleFloat( frame->bounds[0][j] );
				frame->bounds[1][j] = LittleFloat( frame->bounds[1][j] );
				frame->localOrigin[j] = LittleFloat( frame->localOrigin[j] );
			}
		}
	}

	// swap all the tags
	tag = ( mdcTag_t * )( (byte *)mod->mdc[lod] + mod->mdc[lod]->ofsTags );
	if ( LittleLong( 1 ) != 1 ) {
		for ( i = 0 ; i < mod->mdc[lod]->numTags * mod->mdc[lod]->numFrames ; i++, tag++ ) {
			for ( j = 0 ; j < 3 ; j++ ) {
				tag->xyz[j] = LittleShort( tag->xyz[j] );
				tag->angles[j] = LittleShort( tag->angles[j] );
			}
		}
	}

	// swap all the surfaces
	surf = ( mdcSurface_t * )( (byte *)mod->mdc[lod] + mod->mdc[lod]->ofsSurfaces );
	for ( i = 0 ; i < mod->mdc[lod]->numSurfaces ; i++ ) {

		LL( surf->ident );
		LL( surf->flags );
		LL( surf->numBaseFrames );
		LL( surf->numCompFrames );
		LL( surf->numShaders );
		LL( surf->numTriangles );
		LL( surf->ofsTriangles );
		LL( surf->numVerts );
		LL( surf->ofsShaders );
		LL( surf->ofsSt );
		LL( surf->ofsXyzNormals );
		LL( surf->ofsXyzCompressed );
		LL( surf->ofsFrameBaseFrames );
		LL( surf->ofsFrameCompFrames );
		LL( surf->ofsEnd );

		if ( surf->numVerts > SHADER_MAX_VERTEXES ) {
			ri.Error( ERR_DROP, "R_LoadMDC: %s has more than %i verts on a surface (%i)",
					  mod_name, SHADER_MAX_VERTEXES, surf->numVerts );
		}
		if ( surf->numTriangles * 3 > SHADER_MAX_INDEXES ) {
			ri.Error( ERR_DROP, "R_LoadMDC: %s has more than %i triangles on a surface (%i)",
					  mod_name, SHADER_MAX_INDEXES / 3, surf->numTriangles );
		}

		// change to surface identifier
		surf->ident = SF_MDC;

		// lowercase the surface name so skin compares are faster
		Q_strlwr( surf->name );

		// strip off a trailing _1 or _2
		// this is a crutch for q3data being a mess
		j = strlen( surf->name );
		if ( j > 2 && surf->name[j - 2] == '_' ) {
			surf->name[j - 2] = 0;
		}

		// register the shaders
		shader = ( md3Shader_t * )( (byte *)surf + surf->ofsShaders );
		for ( j = 0 ; j < surf->numShaders ; j++, shader++ ) {
			shader_t    *sh;

			sh = R_FindShader( shader->name, LIGHTMAP_NONE, qtrue );
			if ( sh->defaultShader ) {
				shader->shaderIndex = 0;
			} else {
				shader->shaderIndex = sh->index;
			}
		}

		// Ridah, optimization, only do the swapping if we really need to
		if ( LittleShort( 1 ) != 1 ) {

			// swap all the triangles
			tri = ( md3Triangle_t * )( (byte *)surf + surf->ofsTriangles );
			for ( j = 0 ; j < surf->numTriangles ; j++, tri++ ) {
				LL( tri->indexes[0] );
				LL( tri->indexes[1] );
				LL( tri->indexes[2] );
			}

			// swap all the ST
			st = ( md3St_t * )( (byte *)surf + surf->ofsSt );
			for ( j = 0 ; j < surf->numVerts ; j++, st++ ) {
				st->st[0] = LittleFloat( st->st[0] );
				st->st[1] = LittleFloat( st->st[1] );
			}

			// swap all the XyzNormals
			xyz = ( md3XyzNormal_t * )( (byte *)surf + surf->ofsXyzNormals );
			for ( j = 0 ; j < surf->numVerts * surf->numBaseFrames ; j++, xyz++ )
			{
				xyz->xyz[0] = LittleShort( xyz->xyz[0] );
				xyz->xyz[1] = LittleShort( xyz->xyz[1] );
				xyz->xyz[2] = LittleShort( xyz->xyz[2] );

				xyz->normal = LittleShort( xyz->normal );
			}

			// swap all the XyzCompressed
			xyzComp = ( mdcXyzCompressed_t * )( (byte *)surf + surf->ofsXyzCompressed );
			for ( j = 0 ; j < surf->numVerts * surf->numCompFrames ; j++, xyzComp++ )
			{
				LL( xyzComp->ofsVec );
			}

			// swap the frameBaseFrames
			ps = ( short * )( (byte *)surf + surf->ofsFrameBaseFrames );
			for ( j = 0; j < mod->mdc[lod]->numFrames; j++, ps++ )
			{
				*ps = LittleShort( *ps );
			}

			// swap the frameCompFrames
			ps = ( short * )( (byte *)surf + surf->ofsFrameCompFrames );
			for ( j = 0; j < mod->mdc[lod]->numFrames; j++, ps++ )
			{
				*ps = LittleShort( *ps );
			}
		}
		// done.

		// find the next surface
		surf = ( mdcSurface_t * )( (byte *)surf + surf->ofsEnd );
	}

	return qtrue;
}

// done.
//-------------------------------------------------------------------------------

/*
=================
R_LoadMD3
=================
*/
static qboolean R_LoadMD3( model_t *mod, int lod, void *buffer, const char *mod_name ) {
	int i, j;
	md3Header_t         *pinmodel;
	md3Frame_t          *frame;
	md3Surface_t        *surf;
	md3Shader_t         *shader;
	md3Triangle_t       *tri;
	md3St_t             *st;
	md3XyzNormal_t      *xyz;
	md3Tag_t            *tag;
	int version;
	int size;
	qboolean fixRadius = qfalse;

	pinmodel = (md3Header_t *)buffer;

	version = LittleLong( pinmodel->version );
	if ( version != MD3_VERSION ) {
		ri.Printf( PRINT_WARNING, "R_LoadMD3: %s has wrong version (%i should be %i)\n",
				   mod_name, version, MD3_VERSION );
		return qfalse;
	}

	mod->type = MOD_MESH;
	size = LittleLong( pinmodel->ofsEnd );
	mod->dataSize += size;
	// Ridah, convert to compressed format
	mod->md3[lod] = ri.Hunk_Alloc( size, h_low );
	
	// done.

	memcpy( mod->md3[lod], buffer, LittleLong( pinmodel->ofsEnd ) );

	LL( mod->md3[lod]->ident );
	LL( mod->md3[lod]->version );
	LL( mod->md3[lod]->numFrames );
	LL( mod->md3[lod]->numTags );
	LL( mod->md3[lod]->numSurfaces );
	LL( mod->md3[lod]->ofsFrames );
	LL( mod->md3[lod]->ofsTags );
	LL( mod->md3[lod]->ofsSurfaces );
	LL( mod->md3[lod]->ofsEnd );

	if ( mod->md3[lod]->numFrames < 1 ) {
		ri.Printf( PRINT_WARNING, "R_LoadMD3: %s has no frames\n", mod_name );
		return qfalse;
	}

	if ( strstr( mod->name,"sherman" ) || strstr( mod->name, "mg42" ) ) {
		fixRadius = qtrue;
	}

	// swap all the frames
	frame = ( md3Frame_t * )( (byte *)mod->md3[lod] + mod->md3[lod]->ofsFrames );
	for ( i = 0 ; i < mod->md3[lod]->numFrames ; i++, frame++ ) {
		frame->radius = LittleFloat( frame->radius );
		if ( fixRadius ) {
			frame->radius = 256;
			for ( j = 0 ; j < 3 ; j++ ) {
				frame->bounds[0][j] = 128;
				frame->bounds[1][j] = -128;
				frame->localOrigin[j] = LittleFloat( frame->localOrigin[j] );
			}
		}
		// Hack for Bug using plugin generated model
		else if ( frame->radius == 1 ) {
			frame->radius = 256;
			for ( j = 0 ; j < 3 ; j++ ) {
				frame->bounds[0][j] = 128;
				frame->bounds[1][j] = -128;
				frame->localOrigin[j] = LittleFloat( frame->localOrigin[j] );
			}
		} else
		{
			for ( j = 0 ; j < 3 ; j++ ) {
				frame->bounds[0][j] = LittleFloat( frame->bounds[0][j] );
				frame->bounds[1][j] = LittleFloat( frame->bounds[1][j] );
				frame->localOrigin[j] = LittleFloat( frame->localOrigin[j] );
			}
		}
	}

	// swap all the tags
	tag = ( md3Tag_t * )( (byte *)mod->md3[lod] + mod->md3[lod]->ofsTags );
	for ( i = 0 ; i < mod->md3[lod]->numTags * mod->md3[lod]->numFrames ; i++, tag++ ) {
		for ( j = 0 ; j < 3 ; j++ ) {
			tag->origin[j] = LittleFloat( tag->origin[j] );
			tag->axis[0][j] = LittleFloat( tag->axis[0][j] );
			tag->axis[1][j] = LittleFloat( tag->axis[1][j] );
			tag->axis[2][j] = LittleFloat( tag->axis[2][j] );
		}
	}

	// swap all the surfaces
	surf = ( md3Surface_t * )( (byte *)mod->md3[lod] + mod->md3[lod]->ofsSurfaces );
	for ( i = 0 ; i < mod->md3[lod]->numSurfaces ; i++ ) {

		LL( surf->ident );
		LL( surf->flags );
		LL( surf->numFrames );
		LL( surf->numShaders );
		LL( surf->numTriangles );
		LL( surf->ofsTriangles );
		LL( surf->numVerts );
		LL( surf->ofsShaders );
		LL( surf->ofsSt );
		LL( surf->ofsXyzNormals );
		LL( surf->ofsEnd );

		if ( surf->numVerts > SHADER_MAX_VERTEXES ) {
			ri.Error( ERR_DROP, "R_LoadMD3: %s has more than %i verts on a surface (%i)",
					  mod_name, SHADER_MAX_VERTEXES, surf->numVerts );
		}
		if ( surf->numTriangles * 3 > SHADER_MAX_INDEXES ) {
			ri.Error( ERR_DROP, "R_LoadMD3: %s has more than %i triangles on a surface (%i)",
					  mod_name, SHADER_MAX_INDEXES / 3, surf->numTriangles );
		}

		// change to surface identifier
		surf->ident = SF_MD3;

		// lowercase the surface name so skin compares are faster
		Q_strlwr( surf->name );

		// strip off a trailing _1 or _2
		// this is a crutch for q3data being a mess
		j = strlen( surf->name );
		if ( j > 2 && surf->name[j - 2] == '_' ) {
			surf->name[j - 2] = 0;
		}

		// register the shaders
		shader = ( md3Shader_t * )( (byte *)surf + surf->ofsShaders );
		for ( j = 0 ; j < surf->numShaders ; j++, shader++ ) {
			shader_t    *sh;

			sh = R_FindShader( shader->name, LIGHTMAP_NONE, qtrue );
			if ( sh->defaultShader ) {
				shader->shaderIndex = 0;
			} else {
				shader->shaderIndex = sh->index;
			}
		}

		// Ridah, optimization, only do the swapping if we really need to
		if ( LittleShort( 1 ) != 1 ) {

			// swap all the triangles
			tri = ( md3Triangle_t * )( (byte *)surf + surf->ofsTriangles );
			for ( j = 0 ; j < surf->numTriangles ; j++, tri++ ) {
				LL( tri->indexes[0] );
				LL( tri->indexes[1] );
				LL( tri->indexes[2] );
			}

			// swap all the ST
			st = ( md3St_t * )( (byte *)surf + surf->ofsSt );
			for ( j = 0 ; j < surf->numVerts ; j++, st++ ) {
				st->st[0] = LittleFloat( st->st[0] );
				st->st[1] = LittleFloat( st->st[1] );
			}

			// swap all the XyzNormals
			xyz = ( md3XyzNormal_t * )( (byte *)surf + surf->ofsXyzNormals );
			for ( j = 0 ; j < surf->numVerts * surf->numFrames ; j++, xyz++ )
			{
				xyz->xyz[0] = LittleShort( xyz->xyz[0] );
				xyz->xyz[1] = LittleShort( xyz->xyz[1] );
				xyz->xyz[2] = LittleShort( xyz->xyz[2] );

				xyz->normal = LittleShort( xyz->normal );
			}

		}
		// done.

		// find the next surface
		surf = ( md3Surface_t * )( (byte *)surf + surf->ofsEnd );
	}

	return qtrue;
}



/*
=================
R_LoadMDS
=================
*/
static qboolean R_LoadMDS( model_t *mod, void *buffer, const char *mod_name ) {
	int i, j, k;
	mdsHeader_t         *pinmodel, *mds;
	mdsFrame_t          *frame;
	mdsSurface_t        *surf;
	mdsTriangle_t       *tri;
	mdsVertex_t         *v;
	mdsBoneInfo_t       *bi;
	mdsTag_t            *tag;
	int version;
	int size;
	shader_t            *sh;
	int frameSize;
	int                 *collapseMap, *boneref;

	pinmodel = (mdsHeader_t *)buffer;

	version = LittleLong( pinmodel->version );
	if ( version != MDS_VERSION ) {
		ri.Printf( PRINT_WARNING, "R_LoadMDS: %s has wrong version (%i should be %i)\n",
				   mod_name, version, MDS_VERSION );
		return qfalse;
	}

	mod->type = MOD_MDS;
	size = LittleLong( pinmodel->ofsEnd );
	mod->dataSize += size;
	mds = mod->mds = ri.Hunk_Alloc( size, h_low );

	memcpy( mds, buffer, LittleLong( pinmodel->ofsEnd ) );

	LL( mds->ident );
	LL( mds->version );
	LL( mds->numFrames );
	LL( mds->numBones );
	LL( mds->numTags );
	LL( mds->numSurfaces );
	LL( mds->ofsFrames );
	LL( mds->ofsBones );
	LL( mds->ofsTags );
	LL( mds->ofsEnd );
	LL( mds->ofsSurfaces );
	mds->lodBias = LittleFloat( mds->lodBias );
	mds->lodScale = LittleFloat( mds->lodScale );
	LL( mds->torsoParent );

	if ( mds->numFrames < 1 ) {
		ri.Printf( PRINT_WARNING, "R_LoadMDS: %s has no frames\n", mod_name );
		return qfalse;
	}

	if ( LittleLong( 1 ) != 1 ) {
		// swap all the frames
		//frameSize = (int)( &((mdsFrame_t *)0)->bones[ mds->numBones ] );
		frameSize = (int) ( sizeof( mdsFrame_t ) - sizeof( mdsBoneFrameCompressed_t ) + mds->numBones * sizeof( mdsBoneFrameCompressed_t ) );
		for ( i = 0 ; i < mds->numFrames ; i++, frame++ ) {
			frame = ( mdsFrame_t * )( (byte *)mds + mds->ofsFrames + i * frameSize );
			frame->radius = LittleFloat( frame->radius );
			for ( j = 0 ; j < 3 ; j++ ) {
				frame->bounds[0][j] = LittleFloat( frame->bounds[0][j] );
				frame->bounds[1][j] = LittleFloat( frame->bounds[1][j] );
				frame->localOrigin[j] = LittleFloat( frame->localOrigin[j] );
				frame->parentOffset[j] = LittleFloat( frame->parentOffset[j] );
			}
			for ( j = 0 ; j < mds->numBones * sizeof( mdsBoneFrameCompressed_t ) / sizeof( short ) ; j++ ) {
				( (short *)frame->bones )[j] = LittleShort( ( (short *)frame->bones )[j] );
			}
		}

		// swap all the tags
		tag = ( mdsTag_t * )( (byte *)mds + mds->ofsTags );
		for ( i = 0 ; i < mds->numTags ; i++, tag++ ) {
			LL( tag->boneIndex );
			tag->torsoWeight = LittleFloat( tag->torsoWeight );
		}

		// swap all the bones
		for ( i = 0 ; i < mds->numBones ; i++, bi++ ) {
			bi = ( mdsBoneInfo_t * )( (byte *)mds + mds->ofsBones + i * sizeof( mdsBoneInfo_t ) );
			LL( bi->parent );
			bi->torsoWeight = LittleFloat( bi->torsoWeight );
			bi->parentDist = LittleFloat( bi->parentDist );
			LL( bi->flags );
		}
	}

	// swap all the surfaces
	surf = ( mdsSurface_t * )( (byte *)mds + mds->ofsSurfaces );
	for ( i = 0 ; i < mds->numSurfaces ; i++ ) {
		if ( LittleLong( 1 ) != 1 ) {
			LL( surf->ident );
			LL( surf->shaderIndex );
			LL( surf->minLod );
			LL( surf->ofsHeader );
			LL( surf->ofsCollapseMap );
			LL( surf->numTriangles );
			LL( surf->ofsTriangles );
			LL( surf->numVerts );
			LL( surf->ofsVerts );
			LL( surf->numBoneReferences );
			LL( surf->ofsBoneReferences );
			LL( surf->ofsEnd );
		}

		if ( surf->numVerts > SHADER_MAX_VERTEXES ) {
			ri.Error( ERR_DROP, "R_LoadMDS: %s has more than %i verts on a surface (%i)",
					  mod_name, SHADER_MAX_VERTEXES, surf->numVerts );
		}
		if ( surf->numTriangles * 3 > SHADER_MAX_INDEXES ) {
			ri.Error( ERR_DROP, "R_LoadMDS: %s has more than %i triangles on a surface (%i)",
					  mod_name, SHADER_MAX_INDEXES / 3, surf->numTriangles );
		}

		// register the shaders
		if ( surf->shader[0] ) {
			sh = R_FindShader( surf->shader, LIGHTMAP_NONE, qtrue );
			if ( sh->defaultShader ) {
				surf->shaderIndex = 0;
			} else {
				surf->shaderIndex = sh->index;
			}
		} else {
			surf->shaderIndex = 0;
		}

		if ( LittleLong( 1 ) != 1 ) {
			// swap all the triangles
			tri = ( mdsTriangle_t * )( (byte *)surf + surf->ofsTriangles );
			for ( j = 0 ; j < surf->numTriangles ; j++, tri++ ) {
				LL( tri->indexes[0] );
				LL( tri->indexes[1] );
				LL( tri->indexes[2] );
			}

			// swap all the vertexes
			v = ( mdsVertex_t * )( (byte *)surf + surf->ofsVerts );
			for ( j = 0 ; j < surf->numVerts ; j++ ) {
				v->normal[0] = LittleFloat( v->normal[0] );
				v->normal[1] = LittleFloat( v->normal[1] );
				v->normal[2] = LittleFloat( v->normal[2] );

				v->texCoords[0] = LittleFloat( v->texCoords[0] );
				v->texCoords[1] = LittleFloat( v->texCoords[1] );

				v->numWeights = LittleLong( v->numWeights );

				for ( k = 0 ; k < v->numWeights ; k++ ) {
					v->weights[k].boneIndex = LittleLong( v->weights[k].boneIndex );
					v->weights[k].boneWeight = LittleFloat( v->weights[k].boneWeight );
					v->weights[k].offset[0] = LittleFloat( v->weights[k].offset[0] );
					v->weights[k].offset[1] = LittleFloat( v->weights[k].offset[1] );
					v->weights[k].offset[2] = LittleFloat( v->weights[k].offset[2] );
				}

				// find the fixedParent for this vert (if exists)
				v->fixedParent = -1;
				if ( v->numWeights == 2 ) {
					// find the closest parent
					if ( VectorLength( v->weights[0].offset ) < VectorLength( v->weights[1].offset ) ) {
						v->fixedParent = 0;
					} else {
						v->fixedParent = 1;
					}
					v->fixedDist = VectorLength( v->weights[v->fixedParent].offset );
				}

				v = (mdsVertex_t *)&v->weights[v->numWeights];
			}

			// swap the collapse map
			collapseMap = ( int * )( (byte *)surf + surf->ofsCollapseMap );
			for ( j = 0; j < surf->numVerts; j++, collapseMap++ ) {
				*collapseMap = LittleLong( *collapseMap );
			}

			// swap the bone references
			boneref = ( int * )( ( byte *)surf + surf->ofsBoneReferences );
			for ( j = 0; j < surf->numBoneReferences; j++, boneref++ ) {
				*boneref = LittleLong( *boneref );
			}
		}

		// find the next surface
		surf = ( mdsSurface_t * )( (byte *)surf + surf->ofsEnd );
	}

	return qtrue;
}




//=============================================================================



/*
===============
R_ModelInit
===============
*/
void R_ModelInit( void ) {
	model_t     *mod;

	// leave a space for NULL model
	tr.numModels = 0;

	mod = R_AllocModel();
	mod->type = MOD_BAD;

}


/*
================
R_Modellist_f
================
*/

void R_Modellist_f( void ) {
	int i, j;
	model_t *mod;
	int total;
	int lods;
	char *filter = Cmd_Argv(1);

	total = 0;
	for ( i = 1 ; i < tr.numModels; i++ ) {
		mod = tr.models[i];

		if(filter[0] != '\0' && !Com_Filter(filter, mod->name, qfalse)){
			continue;
		}
		
		lods = 1;
		for ( j = 1 ; j < MD3_MAX_LODS ; j++ ) {
			if ( mod->md3[j] && mod->md3[j] != mod->md3[j - 1] ) {
				lods++;
			}
		}
		ri.Printf( PRINT_ALL, "%8i : (%i) %s\n",mod->dataSize, lods, mod->name );
		total += mod->dataSize;
	}
	ri.Printf( PRINT_ALL, "%8i : Total models\n", total );

#if 0       // not working right with new hunk
	if ( tr.world ) {
		ri.Printf( PRINT_ALL, "\n%8i : %s\n", tr.world->dataSize, tr.world->name );
	}
#endif
}


//=============================================================================


/*
================
R_GetTag
================
*/
static int R_GetTag( byte *mod, int frame, const char *tagName, int startTagIndex, md3Tag_t **outTag ) {
	md3Tag_t        *tag;
	int i;
	md3Header_t     *md3;

	md3 = (md3Header_t *) mod;

	if ( frame >= md3->numFrames ) {
		// it is possible to have a bad frame while changing models, so don't error
		frame = md3->numFrames - 1;
	}

	if ( startTagIndex > md3->numTags ) {
		*outTag = NULL;
		return -1;
	}

	tag = ( md3Tag_t * )( (byte *)mod + md3->ofsTags ) + frame * md3->numTags;
	for ( i = 0 ; i < md3->numTags ; i++, tag++ ) {
		if ( ( i >= startTagIndex ) && !strcmp( tag->name, tagName ) ) {

			// if we are looking for an indexed tag, wait until we find the correct number of matches
			//if (startTagIndex) {
			//	startTagIndex--;
			//	continue;
			//}

			*outTag = tag;
			return i;   // found it
		}
	}

	*outTag = NULL;
	return -1;
}

/*
================
R_GetMDCTag
================
*/
static int R_GetMDCTag( byte *mod, int frame, const char *tagName, int startTagIndex, mdcTag_t **outTag ) {
	mdcTag_t        *tag;
	mdcTagName_t    *pTagName;
	int i;
	mdcHeader_t     *mdc;

	mdc = (mdcHeader_t *) mod;

	if ( frame >= mdc->numFrames ) {
		// it is possible to have a bad frame while changing models, so don't error
		frame = mdc->numFrames - 1;
	}

	if ( startTagIndex > mdc->numTags ) {
		*outTag = NULL;
		return -1;
	}

	pTagName = ( mdcTagName_t * )( (byte *)mod + mdc->ofsTagNames );
	for ( i = 0 ; i < mdc->numTags ; i++, pTagName++ ) {
		if ( ( i >= startTagIndex ) && !strcmp( pTagName->name, tagName ) ) {
			break;  // found it
		}
	}

	if ( i >= mdc->numTags ) {
		*outTag = NULL;
		return -1;
	}

	tag = ( mdcTag_t * )( (byte *)mod + mdc->ofsTags ) + frame * mdc->numTags + i;
	*outTag = tag;
	return i;
}

/*
================
R_LerpTag

  returns the index of the tag it found, for cycling through tags with the same name
================
*/
int R_LerpTag( orientation_t *tag, const refEntity_t *refent, const char *tagNameIn, int startIndex ) {
	md3Tag_t    *start, *end;
	md3Tag_t ustart, uend;
	int i;
	float frontLerp, backLerp;
	model_t     *model;
	vec3_t sangles, eangles;
	char tagName[MAX_QPATH];       //, *ch;
	int retval;
	qhandle_t handle;
	int startFrame, endFrame;
	float frac;

	handle = refent->hModel;
	startFrame = refent->oldframe;
	endFrame = refent->frame;
	frac = 1.0 - refent->backlerp;

	Q_strncpyz( tagName, tagNameIn, MAX_QPATH );
/*
	// if the tagName has a space in it, then it is passing through the starting tag number
	if (ch = strrchr(tagName, ' ')) {
		*ch = 0;
		ch++;
		startIndex = atoi(ch);
	}
*/
	model = R_GetModelByHandle( handle );
	if ( !model->md3[0] && !model->mdc[0] && !model->mds ) {
		AxisClear( tag->axis );
		VectorClear( tag->origin );
		return -1;
	}

	frontLerp = frac;
	backLerp = 1.0 - frac;

	if ( model->type == MOD_MESH ) {
		// old MD3 style
		retval = R_GetTag( (byte *)model->md3[0], startFrame, tagName, startIndex, &start );
		retval = R_GetTag( (byte *)model->md3[0], endFrame, tagName, startIndex, &end );

	} else if ( model->type == MOD_MDS ) {    // use bone lerping

		retval = R_GetBoneTag( tag, model->mds, startIndex, refent, tagNameIn );

		if ( retval >= 0 ) {
			return retval;
		}

		// failed
		return -1;

	} else {
		// psuedo-compressed MDC tags
		mdcTag_t    *cstart, *cend;

		retval = R_GetMDCTag( (byte *)model->mdc[0], startFrame, tagName, startIndex, &cstart );
		retval = R_GetMDCTag( (byte *)model->mdc[0], endFrame, tagName, startIndex, &cend );

		// uncompress the MDC tags into MD3 style tags
		if ( cstart && cend ) {
			for ( i = 0; i < 3; i++ ) {
				ustart.origin[i] = (float)cstart->xyz[i] * MD3_XYZ_SCALE;
				uend.origin[i] = (float)cend->xyz[i] * MD3_XYZ_SCALE;
				sangles[i] = (float)cstart->angles[i] * MDC_TAG_ANGLE_SCALE;
				eangles[i] = (float)cend->angles[i] * MDC_TAG_ANGLE_SCALE;
			}

			AnglesToAxis( sangles, ustart.axis );
			AnglesToAxis( eangles, uend.axis );

			start = &ustart;
			end = &uend;
		} else {
			start = NULL;
			end = NULL;
		}

	}

	if ( !start || !end ) {
		AxisClear( tag->axis );
		VectorClear( tag->origin );
		return -1;
	}

	for ( i = 0 ; i < 3 ; i++ ) {
		tag->origin[i] = start->origin[i] * backLerp +  end->origin[i] * frontLerp;
		tag->axis[0][i] = start->axis[0][i] * backLerp +  end->axis[0][i] * frontLerp;
		tag->axis[1][i] = start->axis[1][i] * backLerp +  end->axis[1][i] * frontLerp;
		tag->axis[2][i] = start->axis[2][i] * backLerp +  end->axis[2][i] * frontLerp;
	}

	VectorNormalize( tag->axis[0] );
	VectorNormalize( tag->axis[1] );
	VectorNormalize( tag->axis[2] );

	return retval;
}



/*
====================
R_ModelBounds
====================
*/
void R_ModelBounds( qhandle_t handle, vec3_t mins, vec3_t maxs ) {
	model_t     *model;
	md3Header_t *header;
	md3Frame_t  *frame;

	model = R_GetModelByHandle( handle );

	if ( model->bmodel ) {
		VectorCopy( model->bmodel->bounds[0], mins );
		VectorCopy( model->bmodel->bounds[1], maxs );
		return;
	}

	// Ridah
	if ( model->md3[0] ) {
		header = model->md3[0];

		frame = ( md3Frame_t * )( (byte *)header + header->ofsFrames );

		VectorCopy( frame->bounds[0], mins );
		VectorCopy( frame->bounds[1], maxs );
		return;
	} else if ( model->mdc[0] ) {
		frame = ( md3Frame_t * )( (byte *)model->mdc[0] + model->mdc[0]->ofsFrames );

		VectorCopy( frame->bounds[0], mins );
		VectorCopy( frame->bounds[1], maxs );
		return;
	}

	VectorClear( mins );
	VectorClear( maxs );
	// done.
}



