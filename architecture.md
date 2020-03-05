# Top level
* config parsing - `config.json` and `scene.h`
* integrator - handles scheduling threads and rendering both the scene and calculating/rendering a given pixel. this should also include measuring variance for a chunk and increasing sample counts or subdividing chunks to decrease variance. each integrator should also handle differing methods or orderings of rendering the scene's chunks
    * Path Tracing (PT)
    * Bidirectional Path Tracing (BDPT)
    * Stochastic Progressive Photon Mapping (SPPM)
    * Metropolis Light Transport (MLT)
    * Vertex Connection Merging (VCM)

# lower level
* common code for materials and pdfs