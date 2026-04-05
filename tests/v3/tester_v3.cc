/*
 * tester_v3.cc — Unit tests for tiny_gltf_v3.h (v3 API), including
 *               the tg3_validate() validation feature.
 */

#define TINYGLTF3_IMPLEMENTATION
#include "tiny_gltf_v3.h"

#define CATCH_CONFIG_MAIN
#include "../catch.hpp"

#include <cstring>
#include <cstdio>

/* ======================================================================
 * Helpers
 * ====================================================================== */

/* Build a minimal valid glTF 2.0 JSON string */
static const char *MINIMAL_GLTF_JSON =
    "{\"asset\":{\"version\":\"2.0\"}}";

/* Parse a JSON string into a tg3_model, returns error code. */
static tg3_error_code parse_json(tg3_model *model, tg3_error_stack *errors,
                                  const char *json) {
    tg3_parse_options opts;
    tg3_parse_options_init(&opts);
    opts.required_sections = TG3_NO_REQUIRE;
    return tg3_parse(model, errors,
                     (const uint8_t *)json, (uint64_t)strlen(json),
                     "", 0, &opts);
}

/* ======================================================================
 * Tests: tg3_validate() — basic API behaviour
 * ====================================================================== */

TEST_CASE("v3-validate-null-model", "[v3][validate]") {
    tg3_error_stack errors;
    tg3_error_stack_init(&errors);

    tg3_error_code rc = tg3_validate(NULL, &errors);
    REQUIRE(rc == TG3_ERR_INVALID_VALUE);

    tg3_error_stack_free(&errors);
}

TEST_CASE("v3-validate-null-errors", "[v3][validate]") {
    tg3_model model;
    memset(&model, 0, sizeof(model));
    model.default_scene = -1;

    /* errors may be NULL — results are silently discarded */
    tg3_error_code rc = tg3_validate(&model, NULL);
    /* Empty model (no asset.version) should still return an error code */
    REQUIRE(rc != TG3_OK); /* missing asset.version */

    tg3_model_free(&model);
}

TEST_CASE("v3-validate-minimal-valid", "[v3][validate]") {
    tg3_model model;
    tg3_error_stack errors;
    memset(&model, 0, sizeof(model));
    tg3_error_stack_init(&errors);

    tg3_error_code rc = parse_json(&model, &errors, MINIMAL_GLTF_JSON);
    REQUIRE(rc == TG3_OK);

    tg3_error_stack_free(&errors);
    tg3_error_stack_init(&errors);

    rc = tg3_validate(&model, &errors);
    REQUIRE(rc == TG3_OK);
    REQUIRE(tg3_errors_has_error(&errors) == 0);

    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
}

/* ======================================================================
 * Tests: asset.version validation
 * ====================================================================== */

TEST_CASE("v3-validate-missing-asset-version", "[v3][validate][asset]") {
    tg3_model model;
    tg3_error_stack errors;
    memset(&model, 0, sizeof(model));
    tg3_error_stack_init(&errors);

    /* asset.version.data == NULL means absent */
    model.default_scene = -1;
    tg3_error_code rc = tg3_validate(&model, &errors);

    REQUIRE(rc == TG3_ERR_MISSING_REQUIRED);
    REQUIRE(tg3_errors_has_error(&errors) == 1);

    bool found = false;
    for (uint32_t i = 0; i < tg3_errors_count(&errors); i++) {
        const tg3_error_entry *e = tg3_errors_get(&errors, i);
        if (e->code == TG3_ERR_MISSING_REQUIRED) { found = true; break; }
    }
    REQUIRE(found);

    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
}

TEST_CASE("v3-validate-wrong-asset-version-warn", "[v3][validate][asset]") {
    tg3_model model;
    tg3_error_stack errors;
    tg3_error_stack_init(&errors);

    /* Parse a model with version "1.0" */
    const char *json = "{\"asset\":{\"version\":\"1.0\"}}";
    tg3_error_code rc = parse_json(&model, &errors, json);
    REQUIRE(rc == TG3_OK);

    tg3_error_stack_free(&errors);
    tg3_error_stack_init(&errors);

    rc = tg3_validate(&model, &errors);
    /* Wrong version emits a WARNING, not an error */
    REQUIRE(rc == TG3_OK);
    REQUIRE(tg3_errors_has_error(&errors) == 0);

    bool found_warn = false;
    for (uint32_t i = 0; i < tg3_errors_count(&errors); i++) {
        const tg3_error_entry *e = tg3_errors_get(&errors, i);
        if (e->severity == TG3_SEVERITY_WARNING &&
            e->code    == TG3_ERR_INVALID_VALUE) {
            found_warn = true; break;
        }
    }
    REQUIRE(found_warn);

    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
}

/* ======================================================================
 * Tests: default scene index
 * ====================================================================== */

TEST_CASE("v3-validate-invalid-default-scene", "[v3][validate][scene]") {
    tg3_model model;
    tg3_error_stack errors;
    tg3_error_stack_init(&errors);

    parse_json(&model, &errors, MINIMAL_GLTF_JSON);
    tg3_error_stack_free(&errors);
    tg3_error_stack_init(&errors);

    /* Point default_scene to a non-existent scene */
    model.default_scene = 5;

    tg3_error_code rc = tg3_validate(&model, &errors);
    REQUIRE(rc == TG3_ERR_INVALID_INDEX);
    REQUIRE(tg3_errors_has_error(&errors) == 1);

    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
}

/* ======================================================================
 * Tests: accessor validation
 * ====================================================================== */

TEST_CASE("v3-validate-accessor-invalid-component-type", "[v3][validate][accessor]") {
    tg3_model model;
    tg3_error_stack errors;
    tg3_error_stack_init(&errors);

    const char *json =
        "{"
        "\"asset\":{\"version\":\"2.0\"},"
        "\"accessors\":[{"
        "  \"componentType\":9999,"
        "  \"count\":1,"
        "  \"type\":\"SCALAR\""
        "}]"
        "}";
    parse_json(&model, &errors, json);
    tg3_error_stack_free(&errors);
    tg3_error_stack_init(&errors);

    tg3_error_code rc = tg3_validate(&model, &errors);
    REQUIRE(rc == TG3_ERR_INVALID_ACCESSOR);
    REQUIRE(tg3_errors_has_error(&errors) == 1);

    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
}

TEST_CASE("v3-validate-accessor-zero-count", "[v3][validate][accessor]") {
    tg3_model model;
    tg3_error_stack errors;
    tg3_error_stack_init(&errors);

    /* Build model directly */
    parse_json(&model, &errors, MINIMAL_GLTF_JSON);
    tg3_error_stack_free(&errors);
    tg3_error_stack_init(&errors);

    /* Inject a fake accessor with count=0 */
    tg3_accessor *acc = (tg3_accessor *)calloc(1, sizeof(tg3_accessor));
    acc->buffer_view   = -1;
    acc->component_type = TG3_COMPONENT_TYPE_FLOAT;
    acc->type           = TG3_TYPE_SCALAR;
    acc->count          = 0; /* invalid */
    model.accessors       = acc;
    model.accessors_count = 1;

    tg3_error_code rc = tg3_validate(&model, &errors);
    REQUIRE(rc == TG3_ERR_INVALID_ACCESSOR);

    /* Restore so tg3_model_free doesn't double-free */
    model.accessors       = NULL;
    model.accessors_count = 0;
    free(acc);

    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
}

TEST_CASE("v3-validate-accessor-invalid-bufferView-index", "[v3][validate][accessor]") {
    tg3_model model;
    tg3_error_stack errors;
    tg3_error_stack_init(&errors);

    const char *json =
        "{"
        "\"asset\":{\"version\":\"2.0\"},"
        "\"accessors\":[{"
        "  \"bufferView\":99,"
        "  \"componentType\":5126,"
        "  \"count\":1,"
        "  \"type\":\"SCALAR\""
        "}]"
        "}";
    parse_json(&model, &errors, json);
    tg3_error_stack_free(&errors);
    tg3_error_stack_init(&errors);

    tg3_error_code rc = tg3_validate(&model, &errors);
    REQUIRE(rc == TG3_ERR_INVALID_INDEX);
    REQUIRE(tg3_errors_has_error(&errors) == 1);

    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
}

/* ======================================================================
 * Tests: mesh validation
 * ====================================================================== */

TEST_CASE("v3-validate-mesh-no-primitives", "[v3][validate][mesh]") {
    tg3_model model;
    tg3_error_stack errors;
    tg3_error_stack_init(&errors);

    const char *json =
        "{"
        "\"asset\":{\"version\":\"2.0\"},"
        "\"meshes\":[{\"primitives\":[]}]"
        "}";
    parse_json(&model, &errors, json);
    tg3_error_stack_free(&errors);
    tg3_error_stack_init(&errors);

    tg3_error_code rc = tg3_validate(&model, &errors);
    REQUIRE(rc == TG3_ERR_INVALID_MESH);
    REQUIRE(tg3_errors_has_error(&errors) == 1);

    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
}

TEST_CASE("v3-validate-mesh-indices-out-of-range", "[v3][validate][mesh]") {
    tg3_model model;
    tg3_error_stack errors;
    tg3_error_stack_init(&errors);

    const char *json =
        "{"
        "\"asset\":{\"version\":\"2.0\"},"
        "\"accessors\":["
        "  {\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"}"
        "],"
        "\"meshes\":[{"
        "  \"primitives\":[{"
        "    \"attributes\":{\"POSITION\":0},"
        "    \"indices\":99"
        "  }]"
        "}]"
        "}";
    parse_json(&model, &errors, json);
    tg3_error_stack_free(&errors);
    tg3_error_stack_init(&errors);

    tg3_error_code rc = tg3_validate(&model, &errors);
    REQUIRE(rc == TG3_ERR_INVALID_INDEX);
    REQUIRE(tg3_errors_has_error(&errors) == 1);

    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
}

/* ======================================================================
 * Tests: node validation
 * ====================================================================== */

TEST_CASE("v3-validate-node-invalid-mesh-index", "[v3][validate][node]") {
    tg3_model model;
    tg3_error_stack errors;
    tg3_error_stack_init(&errors);

    const char *json =
        "{"
        "\"asset\":{\"version\":\"2.0\"},"
        "\"nodes\":[{\"mesh\":42}]"
        "}";
    parse_json(&model, &errors, json);
    tg3_error_stack_free(&errors);
    tg3_error_stack_init(&errors);

    tg3_error_code rc = tg3_validate(&model, &errors);
    REQUIRE(rc == TG3_ERR_INVALID_INDEX);
    REQUIRE(tg3_errors_has_error(&errors) == 1);

    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
}

TEST_CASE("v3-validate-node-self-reference", "[v3][validate][node]") {
    tg3_model model;
    tg3_error_stack errors;
    tg3_error_stack_init(&errors);

    const char *json =
        "{"
        "\"asset\":{\"version\":\"2.0\"},"
        "\"nodes\":[{\"children\":[0]}]"
        "}";
    parse_json(&model, &errors, json);
    tg3_error_stack_free(&errors);
    tg3_error_stack_init(&errors);

    tg3_error_code rc = tg3_validate(&model, &errors);
    REQUIRE(rc == TG3_ERR_INVALID_NODE);
    REQUIRE(tg3_errors_has_error(&errors) == 1);

    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
}

/* ======================================================================
 * Tests: texture validation
 * ====================================================================== */

TEST_CASE("v3-validate-texture-invalid-source", "[v3][validate][texture]") {
    tg3_model model;
    tg3_error_stack errors;
    tg3_error_stack_init(&errors);

    const char *json =
        "{"
        "\"asset\":{\"version\":\"2.0\"},"
        "\"textures\":[{\"source\":99}]"
        "}";
    parse_json(&model, &errors, json);
    tg3_error_stack_free(&errors);
    tg3_error_stack_init(&errors);

    tg3_error_code rc = tg3_validate(&model, &errors);
    REQUIRE(rc == TG3_ERR_INVALID_INDEX);
    REQUIRE(tg3_errors_has_error(&errors) == 1);

    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
}

/* ======================================================================
 * Tests: material validation
 * ====================================================================== */

TEST_CASE("v3-validate-material-invalid-alpha-mode", "[v3][validate][material]") {
    tg3_model model;
    tg3_error_stack errors;
    tg3_error_stack_init(&errors);

    const char *json =
        "{"
        "\"asset\":{\"version\":\"2.0\"},"
        "\"materials\":[{\"alphaMode\":\"INVALID\"}]"
        "}";
    parse_json(&model, &errors, json);
    tg3_error_stack_free(&errors);
    tg3_error_stack_init(&errors);

    tg3_error_code rc = tg3_validate(&model, &errors);
    REQUIRE(rc == TG3_ERR_INVALID_MATERIAL);
    REQUIRE(tg3_errors_has_error(&errors) == 1);

    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
}

/* ======================================================================
 * Tests: camera validation
 * ====================================================================== */

TEST_CASE("v3-validate-camera-perspective-bad-yfov", "[v3][validate][camera]") {
    tg3_model model;
    tg3_error_stack errors;
    tg3_error_stack_init(&errors);

    const char *json =
        "{"
        "\"asset\":{\"version\":\"2.0\"},"
        "\"cameras\":[{"
        "  \"type\":\"perspective\","
        "  \"perspective\":{\"yfov\":0.0,\"znear\":0.01,\"zfar\":100.0,"
        "                   \"aspectRatio\":1.0}"
        "}]"
        "}";
    parse_json(&model, &errors, json);
    tg3_error_stack_free(&errors);
    tg3_error_stack_init(&errors);

    tg3_error_code rc = tg3_validate(&model, &errors);
    REQUIRE(rc == TG3_ERR_INVALID_CAMERA);
    REQUIRE(tg3_errors_has_error(&errors) == 1);

    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
}

TEST_CASE("v3-validate-camera-invalid-type", "[v3][validate][camera]") {
    tg3_model model;
    tg3_error_stack errors;
    tg3_error_stack_init(&errors);

    const char *json =
        "{"
        "\"asset\":{\"version\":\"2.0\"},"
        "\"cameras\":[{\"type\":\"unknown\"}]"
        "}";
    parse_json(&model, &errors, json);
    tg3_error_stack_free(&errors);
    tg3_error_stack_init(&errors);

    tg3_error_code rc = tg3_validate(&model, &errors);
    REQUIRE(rc == TG3_ERR_INVALID_CAMERA);
    REQUIRE(tg3_errors_has_error(&errors) == 1);

    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
}

/* ======================================================================
 * Tests: scene validation
 * ====================================================================== */

TEST_CASE("v3-validate-scene-invalid-node-index", "[v3][validate][scene]") {
    tg3_model model;
    tg3_error_stack errors;
    tg3_error_stack_init(&errors);

    const char *json =
        "{"
        "\"asset\":{\"version\":\"2.0\"},"
        "\"scenes\":[{\"nodes\":[99]}]"
        "}";
    parse_json(&model, &errors, json);
    tg3_error_stack_free(&errors);
    tg3_error_stack_init(&errors);

    tg3_error_code rc = tg3_validate(&model, &errors);
    REQUIRE(rc == TG3_ERR_INVALID_SCENE);
    REQUIRE(tg3_errors_has_error(&errors) == 1);

    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
}

/* ======================================================================
 * Tests: animation validation
 * ====================================================================== */

TEST_CASE("v3-validate-animation-invalid-sampler-ref", "[v3][validate][animation]") {
    tg3_model model;
    tg3_error_stack errors;
    tg3_error_stack_init(&errors);

    const char *json =
        "{"
        "\"asset\":{\"version\":\"2.0\"},"
        "\"accessors\":["
        "  {\"componentType\":5126,\"count\":2,\"type\":\"SCALAR\"},"
        "  {\"componentType\":5126,\"count\":2,\"type\":\"VEC3\"}"
        "],"
        "\"animations\":[{"
        "  \"samplers\":[{\"input\":0,\"output\":1,\"interpolation\":\"LINEAR\"}],"
        "  \"channels\":[{"
        "    \"sampler\":99,"
        "    \"target\":{\"path\":\"translation\"}"
        "  }]"
        "}]"
        "}";
    parse_json(&model, &errors, json);
    tg3_error_stack_free(&errors);
    tg3_error_stack_init(&errors);

    tg3_error_code rc = tg3_validate(&model, &errors);
    REQUIRE(rc == TG3_ERR_INVALID_INDEX);
    REQUIRE(tg3_errors_has_error(&errors) == 1);

    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
}

TEST_CASE("v3-validate-animation-invalid-target-path", "[v3][validate][animation]") {
    tg3_model model;
    tg3_error_stack errors;
    tg3_error_stack_init(&errors);

    const char *json =
        "{"
        "\"asset\":{\"version\":\"2.0\"},"
        "\"accessors\":["
        "  {\"componentType\":5126,\"count\":2,\"type\":\"SCALAR\"},"
        "  {\"componentType\":5126,\"count\":2,\"type\":\"VEC3\"}"
        "],"
        "\"animations\":[{"
        "  \"samplers\":[{\"input\":0,\"output\":1,\"interpolation\":\"LINEAR\"}],"
        "  \"channels\":[{"
        "    \"sampler\":0,"
        "    \"target\":{\"path\":\"bogus\"}"
        "  }]"
        "}]"
        "}";
    parse_json(&model, &errors, json);
    tg3_error_stack_free(&errors);
    tg3_error_stack_init(&errors);

    tg3_error_code rc = tg3_validate(&model, &errors);
    REQUIRE(rc == TG3_ERR_INVALID_VALUE);
    REQUIRE(tg3_errors_has_error(&errors) == 1);

    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
}

/* ======================================================================
 * Tests: error stack msg_arena_ lifetime
 * ====================================================================== */

TEST_CASE("v3-validate-error-messages-persist", "[v3][validate][errorstack]") {
    tg3_model model;
    tg3_error_stack errors;
    tg3_error_stack_init(&errors);

    /* Trigger multiple validation errors */
    const char *json =
        "{"
        "\"asset\":{\"version\":\"2.0\"},"
        "\"nodes\":[{\"mesh\":99,\"camera\":88}],"
        "\"textures\":[{\"source\":77}]"
        "}";
    parse_json(&model, &errors, json);
    tg3_error_stack_free(&errors);
    tg3_error_stack_init(&errors);

    tg3_validate(&model, &errors);

    uint32_t count = tg3_errors_count(&errors);
    REQUIRE(count >= 3);

    /* All message and path pointers must be non-NULL and readable */
    for (uint32_t i = 0; i < count; i++) {
        const tg3_error_entry *e = tg3_errors_get(&errors, i);
        REQUIRE(e != NULL);
        REQUIRE(e->message != NULL);
        /* Sanity: message should not be empty */
        REQUIRE(strlen(e->message) > 0);
    }

    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
}

/* ======================================================================
 * Regression tests for bugs found in deep review
 * ====================================================================== */

/* Bug: tg3__val_check_tex_index bypassed TG3__VERR so first_err was never
 * updated when a material's texture index was out of range.  tg3_validate()
 * was returning TG3_OK despite errors having been pushed to the stack. */
TEST_CASE("v3-validate-material-invalid-tex-index-returns-error", "[v3][validate][material][regression]") {
    tg3_model model;
    tg3_error_stack errors;
    tg3_error_stack_init(&errors);

    /* Material references texture index 5 but there are no textures */
    const char *json =
        "{"
        "\"asset\":{\"version\":\"2.0\"},"
        "\"materials\":[{"
        "  \"pbrMetallicRoughness\":{"
        "    \"baseColorTexture\":{\"index\":5}"
        "  }"
        "}]"
        "}";
    parse_json(&model, &errors, json);
    tg3_error_stack_free(&errors);
    tg3_error_stack_init(&errors);

    tg3_error_code rc = tg3_validate(&model, &errors);
    /* Must return a non-OK error code, not TG3_OK */
    REQUIRE(rc == TG3_ERR_INVALID_INDEX);
    REQUIRE(tg3_errors_has_error(&errors) == 1);

    /* Verify the error is about the texture index */
    bool found = false;
    for (uint32_t i = 0; i < tg3_errors_count(&errors); i++) {
        const tg3_error_entry *e = tg3_errors_get(&errors, i);
        if (e->code == TG3_ERR_INVALID_INDEX &&
            e->severity == TG3_SEVERITY_ERROR) {
            found = true;
            break;
        }
    }
    REQUIRE(found);

    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
}

/* Bug: buffer-view byte-bounds check compared against buf->data.count which
 * is 0 when the buffer's external file was never loaded (no FS callbacks).
 * Every buffer view would incorrectly be flagged as out-of-range. */
TEST_CASE("v3-validate-bufview-bounds-unloaded-buffer-no-false-positive", "[v3][validate][bufview][regression]") {
    tg3_model model;
    tg3_error_stack errors;
    tg3_error_stack_init(&errors);

    /* Parse a model that references an external buffer (not a data URI).
     * Without FS callbacks the buffer data is never loaded (data.count==0).
     * The validator must NOT report a bounds error for the buffer view. */
    const char *json =
        "{"
        "\"asset\":{\"version\":\"2.0\"},"
        "\"buffers\":[{\"byteLength\":48,\"uri\":\"external.bin\"}],"
        "\"bufferViews\":[{"
        "  \"buffer\":0,"
        "  \"byteOffset\":0,"
        "  \"byteLength\":48"
        "}]"
        "}";
    /* Parse without FS support — buffer data stays unloaded */
    parse_json(&model, &errors, json);
    tg3_error_stack_free(&errors);
    tg3_error_stack_init(&errors);

    tg3_error_code rc = tg3_validate(&model, &errors);

    /* Buffer data not loaded: no bounds error should be emitted */
    bool has_bounds_error = false;
    for (uint32_t i = 0; i < tg3_errors_count(&errors); i++) {
        const tg3_error_entry *e = tg3_errors_get(&errors, i);
        if (e->code == TG3_ERR_INVALID_BUFFER_VIEW &&
            e->severity == TG3_SEVERITY_ERROR) {
            has_bounds_error = true;
            break;
        }
    }
    REQUIRE_FALSE(has_bounds_error);
    /* Overall result should be OK (no errors, buffer index valid) */
    REQUIRE(rc == TG3_OK);

    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
}

/* ======================================================================
 * Tests: C++ wrapper
 * ====================================================================== */

TEST_CASE("v3-validate-cpp-wrapper", "[v3][validate][cpp]") {
    tinygltf3::Model model;
    tinygltf3::ErrorStack errors;

    const char *json = "{\"asset\":{\"version\":\"2.0\"}}";
    tg3_parse_options opts;
    tg3_parse_options_init(&opts);
    opts.required_sections = TG3_NO_REQUIRE;
    tg3_parse_auto(model.get(), errors.get(),
                   (const uint8_t *)json, strlen(json),
                   "", 0, &opts);

    tg3_error_code rc = tinygltf3::validate(model, errors);
    REQUIRE(rc == TG3_OK);
    REQUIRE(errors.has_error() == false);
}
