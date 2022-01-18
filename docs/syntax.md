
# Syntax brainstorm 

## ki.json

```json
{
    // optional
    // In case you want to make a package for others
    "package": {
        "requirements": {
            // If people want to install your package
            // They also need to install these
            "packages": {
                // Use either name or git repo or both
                // If both, it will look for 1 of them
                "name": "z-orm",
                "src": "github.com/someone/z-orm",
                // Version control
                "version-min": "1.3.0", // optional
                "version-max": "1.8.*" // optional
            }
        }
    },
    // Tell the compiler where to find the files for
    // a certain namespace
    "namespaces": {
        "main": "src",
        "models": "src/models"
    },
    // Packages for your project
    "packages": {
        "orm": { // custom name to use in code
            "src": "github.com/someone/z-orm",
            "version": "1.5.1"
        },
        "paypal": {
            "name": "paypal-sdk",
            "version": "1.12.*"
        },
        "something": {
            "src": "../other-project",
        }
    }
}
```


# Http server

```
func handle_request(ki:http:Request req, ki:http:Response res) {

    if req.path == "/" {
        res.send("Hello")
    }
}

func main() {

    server := ki:http:Server {
        port: 8000
        on_request: handle_request
    }

    server.start()
}
```

## Gui

```

use ki:gui:*

func main() {

    render := Render {}
    window := Window {
        width: 800
        height: 450
        show: true
    }

    render.add_window(window)

    grid := Grid {
        show: true,
        x: 0,
        y: 0,
        width: window.width,
        height: window.height,
        rows: [
            Row {
                background: "#ffffff"
                color: "#000000"
                columns: [
                    Column {
                        styles: [style_content]
                        content: [
                            Textbox {
                                text: ""
                                placeholder: "Your name"
                            },
                            Button {
                                text: "Test"
                                on_click: func (button) {
                                    window := button.get_window()
                                    window.add_messagebox(MessageBox{
                                        type: "info"
                                        text: "Hello world"
                                    })
                                }
                            }
                        ]
                    }
                ]
            }
        ]
    }
    window.add_grid(grid)

    window.on_resize = func (window) {
        grid := window.grids[0]
        grid.width = window.width;
        grid.height = window.height;
        grid.on_resize()
    }

    window.on_close = function(window) {
        window.renderer.stop = true
    }

    while(true) {
        render.update()
        if render.stop {
            break
        }
        ki:thread:sleep(1000/60)
    }

}
```

## Classes 

```
trait DbModel {

    func save() ! {
        ...
    }
}

class User {

    use DbModel:private save;

    string firstname;
    string lastname;

    func get_fullname() string {
        return this.firstname + " " + this.lastname;
    }

    func get_posts() !Post[] {
        ...
    }

    func get_last_order() !?Order {
        ...
    }
    
    func save_() ! {
        this.firstname = ucfirst(this.firstname);
        this.lastname = ucfirst(this.lastname);
        this.save() or pass;
    }
}
```