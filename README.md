# Shattered Moon Engine

A DirectX 12-based game engine focused on procedural world generation and large-scale exploration.

## Features

### Core Systems
- **ECS Architecture** - Data-oriented Entity-Component-System design
- **Memory Management** - Pool allocators and object pooling
- **Resource Management** - Async asset loading system

### Rendering (DirectX 12)
- Modern DX12 rendering pipeline
- Instanced rendering for large-scale environments
- PBR materials and dynamic lighting (planned)

### Procedural Content Generation (PCG)
- Noise-based terrain generation (Perlin/Simplex)
- Chunk-based world streaming
- Procedural object placement
- LOD system for terrain

### Development Tools
- **gendev CLI** - Intelligent development assistant
  - `gendev ask` - Query project documentation
  - `gendev create` - Generate ECS boilerplate code
- **Editor** - Dear ImGui-based development tools
  - 3D viewport with camera controls
  - Real-time PCG parameter adjustment

## Project Structure

```
Shattered_Moon/
├── src/
│   ├── core/           # Core systems (memory, resources)
│   ├── ecs/            # Entity-Component-System
│   │   ├── components/
│   │   ├── systems/
│   │   └── events/
│   ├── renderer/       # DirectX 12 rendering
│   ├── pcg/            # Procedural generation
│   ├── gameplay/       # Game systems
│   └── editor/         # Development tools
├── gendev/             # CLI tool source
├── assets/             # Game resources
├── docs/               # Documentation
└── tests/              # Unit tests
```

## Requirements

- Windows 10/11
- Visual Studio 2022
- DirectX 12 compatible GPU
- CMake 3.20+
- Windows SDK

## Building

```bash
# Clone the repository
git clone https://github.com/kunho817/Shattered_Moon.git
cd Shattered_Moon

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -G "Visual Studio 17 2022" -A x64

# Build
cmake --build . --config Release
```

## Documentation

- [Project Overview](Project.md) - Detailed project documentation
- [Development Plan](DEVELOPMENT_PLAN.md) - Milestone-based development roadmap

## gendev CLI Usage

```bash
# Query project information
gendev ask "What is the ECS architecture?"

# Generate a new component
gendev create component Position x:float y:float z:float

# Generate a new system
gendev create system Physics

# Generate a new event
gendev create event Collision entity1:int entity2:int
```

## Roadmap

### Phase 1: MVP
- [x] Project setup and documentation
- [ ] Core ECS architecture
- [ ] Basic DX12 rendering
- [ ] Noise-based terrain generation
- [ ] Basic editor with PCG controls

### Phase 2: Enhanced Features
- [ ] PBR rendering pipeline
- [ ] Dynamic lighting and shadows
- [ ] Physics system
- [ ] Scripting system (Lua/Python)

### Phase 3: Advanced Features
- [ ] Procedural structure generation
- [ ] Audio system
- [ ] Animation system

## Inspirations

This engine draws inspiration from:
- **Factorio** - Automation systems
- **Outpost: Infinity Siege** - Base building and defense
- **No Man's Sky** - Vast procedural exploration

## Contributing

Contributions are welcome! Please read the development guidelines in [DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md).

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contact

- GitHub: [@kunho817](https://github.com/kunho817)
