// swift-tools-version: 6.0
import PackageDescription

let package = Package(
    name: "Restime",
    platforms: [
        .macOS(.v13)
    ],
    products: [
        .executable(name: "RestimeMenu", targets: ["RestimeMenu"])
    ],
    targets: [
        .executableTarget(
            name: "RestimeMenu",
            path: "Sources/RestimeMenu"
        ),
        .testTarget(
            name: "RestimeMenuTests",
            dependencies: ["RestimeMenu"],
            path: "Tests/RestimeMenuTests"
        )
    ]
)
