#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/AbcCoreFactory/All.h>

static void
VisitObject(const Alembic::AbcGeom::IObject& obj)
{
  std::string path = obj.getFullName();

  if (path.compare("/") != 0) {
    std::cout << "Object: path = " << path << std::endl;
  }

  Alembic::AbcGeom::ICompoundProperty props = obj.getProperties();
   
  for (size_t i = 0; i < obj.getNumChildren(); i++) {
    const Alembic::AbcGeom::ObjectHeader &header = obj.getChildHeader(i);

    if (Alembic::AbcGeom::IPolyMesh::matches(header)) {
      // Polygon
      Alembic::AbcGeom::IPolyMesh pmesh(obj, header.getName());

      Alembic::AbcGeom::ISampleSelector ss(0.0f, Alembic::AbcGeom::ISampleSelector::kNearIndex);
      Alembic::AbcGeom::IPolyMeshSchema::Sample psample;
      Alembic::AbcGeom::IPolyMeshSchema &ps = pmesh.getSchema();
      
      std::cout << "  # of samples = " << ps.getNumSamples() << std::endl;

      if (ps.getNumSamples() > 0) {
        ps.get(psample,  ss);
        Alembic::Abc::P3fArraySamplePtr P = psample.getPositions();
        std::cout << "  # of positions   = " << P->size() << std::endl;
        std::cout << "  # of face counts = " << psample.getFaceCounts()->size() << std::endl;

      } else {
        std::cout << "Warning: # of samples = 0" << std::endl;
      }
  
    }
  }

}

int
main(
  int argc,
  char **argv)
{
  std::string abc_filename;
  std::string gltf_filename;

  if (argc < 3) {
    std::cerr << "Usage: gltf2abc input.gltf output.abc" << std::endl;
    return EXIT_FAILURE;
  }

  gltf_filename = std::string(argv[1]);
  abc_filename = std::string(argv[2]);

  Alembic::AbcCoreFactory::IFactory factory;
  Alembic::AbcGeom::IArchive archive = factory.getArchive(abc_filename);

  Alembic::AbcGeom::IObject root = archive.getTop();

  std::cout << "# of children " << root.getNumChildren() << std::endl;

  VisitObject(root);

  return EXIT_SUCCESS;
}
