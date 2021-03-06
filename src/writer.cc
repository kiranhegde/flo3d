#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <cassert>
#include <cstdlib>
#include "writer.h"

using namespace std;

//------------------------------------------------------------------------------
// Add integer data
//------------------------------------------------------------------------------
void Writer::attach_vertex_data (vector<double>& data, string name)
{
   vertex_data.push_back (&data);
   vertex_data_name.push_back (name);
}

//------------------------------------------------------------------------------
// Add primitive variables defined at vertices
//------------------------------------------------------------------------------
void Writer::attach_vertex_data (vector<PrimVar>& data)
{
   assert (!has_vertex_primitive);
   vertex_primitive = &data;
   has_vertex_primitive = true;
}

//------------------------------------------------------------------------------
// Add primitive variables defined at cells
//------------------------------------------------------------------------------
void Writer::attach_cell_data (vector<PrimVar>& data)
{
   assert (!has_cell_primitive);
   cell_primitive = &data;
   has_cell_primitive = true;
}

//------------------------------------------------------------------------------
// Specify which variables to write
//------------------------------------------------------------------------------
void Writer::attach_cell_variables (const vector<string>& variables)
{
   if(variables.size() > 0)
      assert (has_cell_primitive);

   for(unsigned int i=0; i<variables.size(); ++i)
      if(variables[i]=="density")
         write_cell_density = true;
      else if(variables[i]=="velocity")
         write_cell_velocity = true;
      else if(variables[i]=="pressure")
         write_cell_pressure = true;
      else if(variables[i]=="mach")
         write_cell_mach = true;
      else
      {
         cout << "Writer: unknown variable " << variables[i] << endl;
         abort ();
      }
}

//------------------------------------------------------------------------------
// Write data to vtk file
//------------------------------------------------------------------------------
void Writer::output_vtk (string filename)
{
   ofstream vtk;
   vtk.open (filename.c_str());

   vtk << "# vtk DataFile Version 3.0" << endl;
   vtk << "flo3d" << endl;
   vtk << "ASCII" << endl;
   vtk << "DATASET UNSTRUCTURED_GRID" << endl;
   vtk << "POINTS  " << grid->n_vertex << "  float" << endl;

   for(unsigned int i=0; i<grid->n_vertex; ++i)
      vtk << grid->vertex[i].x << " " 
          << grid->vertex[i].y << " " 
          << grid->vertex[i].z << endl;

   vtk << "CELLS  " << grid->n_cell << " " << 5 * grid->n_cell << endl;
   for(unsigned int i=0; i<grid->n_cell; ++i)
      vtk << 4 << "  " 
          << grid->cell[i].vertex[0] << " "
          << grid->cell[i].vertex[1] << " "
          << grid->cell[i].vertex[2] << " "
          << grid->cell[i].vertex[3] << endl;

   vtk << "CELL_TYPES  " << grid->n_cell << endl;
   for(unsigned int i=0; i<grid->n_cell; ++i)
      vtk << 10 << endl;

   // Write vertex data
   if(vertex_data.size() > 0 || has_vertex_primitive) 
      vtk << "POINT_DATA  " << grid->n_vertex << endl;

   // Write vertex data to file
   for(unsigned int d=0; d<vertex_data.size(); ++d)
   {
      vtk << "SCALARS  " << vertex_data_name[d] << "  float 1" << endl;
      vtk << "LOOKUP_TABLE default" << endl;
      for(unsigned int i=0; i<grid->n_vertex; ++i)
         vtk << (*vertex_data[d])[i] << endl;
   }

   // If vertex primitive data is available, write to file
   if (has_vertex_primitive)
   {
      vtk << "SCALARS density float 1" << endl;
      vtk << "LOOKUP_TABLE default" << endl;
      for(unsigned int i=0; i<grid->n_vertex; ++i)
         vtk << (*vertex_primitive)[i].density << endl;

      vtk << "SCALARS pressure float 1" << endl;
      vtk << "LOOKUP_TABLE default" << endl;
      for(unsigned int i=0; i<grid->n_vertex; ++i)
         vtk << (*vertex_primitive)[i].pressure << endl;

      vtk << "VECTORS velocity float" << endl;
      for(unsigned int i=0; i<grid->n_vertex; ++i)
         vtk << (*vertex_primitive)[i].velocity.x << "  "
             << (*vertex_primitive)[i].velocity.y << "  "
             << (*vertex_primitive)[i].velocity.z
             << endl;
   }

   // If cell primitive data is available, write to file
   if (has_cell_primitive)
   {
      vtk << "CELL_DATA  " << grid->n_cell << endl;

      if(write_cell_density)
      {
         vtk << "SCALARS density float 1" << endl;
         vtk << "LOOKUP_TABLE default" << endl;
         for(unsigned int i=0; i<grid->n_cell; ++i)
            vtk << (*cell_primitive)[i].density << endl;
      }

      if(write_cell_pressure)
      {
         vtk << "SCALARS pressure float 1" << endl;
         vtk << "LOOKUP_TABLE default" << endl;
         for(unsigned int i=0; i<grid->n_cell; ++i)
            vtk << (*cell_primitive)[i].pressure << endl;
      }

      if(write_cell_velocity)
      {
         vtk << "VECTORS velocity float" << endl;
         for(unsigned int i=0; i<grid->n_cell; ++i)
            vtk << (*cell_primitive)[i].velocity.x << "  "
               << (*cell_primitive)[i].velocity.y << "  "
               << (*cell_primitive)[i].velocity.z
               << endl;
      }

      // Write mach number at cells
      if(write_cell_mach)
      {
         vtk << "SCALARS mach float 1" << endl;
         vtk << "LOOKUP_TABLE default" << endl;
         for(unsigned int i=0; i<grid->n_cell; ++i)
         {
            double sonic_square = material->gamma * 
                                 (*cell_primitive)[i].pressure /
                                 (*cell_primitive)[i].density;
            double mach = sqrt ( (*cell_primitive)[i].velocity.square() /
                                 sonic_square );
            vtk << mach << endl;
         }
      }
   }

   vtk.close ();
}

//------------------------------------------------------------------------------
// Write solution for restarting
//------------------------------------------------------------------------------
void Writer::output_restart ()
{
   assert (has_cell_primitive);

   cout << "Saving restart file flo3d.sol\n";

   ofstream fo;
   fo.open ("flo3d.sol");
   assert (fo.is_open());

   for(unsigned int i=0; i<grid->n_cell; ++i)
      fo << scientific
         << (*cell_primitive)[i].density << "  "
         << (*cell_primitive)[i].velocity.x << "  "
         << (*cell_primitive)[i].velocity.y << "  "
         << (*cell_primitive)[i].velocity.z << "  "
         << (*cell_primitive)[i].pressure   << endl;

   fo.close ();
}
