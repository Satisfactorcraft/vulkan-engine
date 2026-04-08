BUILD_DIR := build
SHADER_DIR := shaders

.PHONY: all build cmake shaders clean rebuild run

all: shaders build

# ── CMake konfigurieren (nur wenn build/ noch nicht existiert) ────────────────
cmake:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake ..

# ── Kompilieren ───────────────────────────────────────────────────────────────
build: cmake
	@cd $(BUILD_DIR) && make -j$(nproc)

# ── Shader kompilieren ────────────────────────────────────────────────────────
shaders:
	@echo "Compiling shaders..."
	@glslc $(SHADER_DIR)/pbr.vert -o $(SHADER_DIR)/pbr.vert.spv
	@glslc $(SHADER_DIR)/pbr.frag -o $(SHADER_DIR)/pbr.frag.spv
	@echo "Shaders done."

# ── Starten ───────────────────────────────────────────────────────────────────
run: all
	@./$(BUILD_DIR)/VulkanEngine

# ── Clean ─────────────────────────────────────────────────────────────────────
clean:
	@rm -rf $(BUILD_DIR)
	@rm -f $(SHADER_DIR)/*.spv
	@echo "Cleaned."

# ── Clean + neu bauen ─────────────────────────────────────────────────────────
rebuild: clean all
