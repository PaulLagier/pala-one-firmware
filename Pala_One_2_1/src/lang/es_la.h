#ifndef PALA_LANG_ES_LA_H
#define PALA_LANG_ES_LA_H

// ============================================================================
//  Spanish (Latin America) string table — es_LA.
//  Mirror of en.h; the key set MUST stay identical. Adding new keys: edit en.h
//  first, then add the same key here. See src/lang/lang.h for the rule.
//
//  Glyph coverage: all accents used here (á é í ó ú ñ ¿ ¡ ü) are in u8g2's
//  Latin Extended (_te) font set already linked by src/ui/font.cpp, so no
//  font change is required. Web responses already declare charset=utf-8.
// ============================================================================

// ----------------------------------------------------------------------------
//  Boot / fatal screens
// ----------------------------------------------------------------------------
#define D_BOOT_STORAGE_ERROR        "Error de almacenamiento"
#define D_BOOT_TRY_FACTORY_RESET    "Pruebe reinicio de fábrica"

// ----------------------------------------------------------------------------
//  About screen
// ----------------------------------------------------------------------------
#define D_ABOUT_HEADER              "Dispositivo"
#define D_ABOUT_FIRMWARE_PREFIX     "Firmware "
#define D_ABOUT_GESTURE_NEXT        "1x siguiente / abajo"
#define D_ABOUT_GESTURE_OPEN        "2x abrir / elegir"
#define D_ABOUT_GESTURE_HOME        "3x inicio"
#define D_ABOUT_GESTURE_BOOKMARK    "Mantener: marcapáginas"

// ----------------------------------------------------------------------------
//  Library menu entries
// ----------------------------------------------------------------------------
#define D_MENU_BOOKMARKS            "Marcapáginas"
#define D_MENU_LIST                 "Lista"
#define D_MENU_APPS                 "Apps"
#define D_MENU_DEVICE               "Dispositivo"
#define D_MENU_UPLOAD               "Conectar"
#define D_LIBRARY_OPEN_FAILED       "Error al abrir"
#define D_LIBRARY_TRY_UPLOAD        "Intente subir de nuevo"

// ----------------------------------------------------------------------------
//  List screen
// ----------------------------------------------------------------------------
#define D_LIST_HEADER               "Lista"
#define D_LIST_NONE                 "Sin elementos"

// ----------------------------------------------------------------------------
//  Upload screen
// ----------------------------------------------------------------------------
#define D_UPLOAD_HEADER             "Subir"
#define D_UPLOAD_WIFI               "Wi-Fi"
#define D_UPLOAD_PASSWORD           "Contraseña"
#define D_UPLOAD_OPEN               "Abrir"

// ----------------------------------------------------------------------------
//  Apps screen
// ----------------------------------------------------------------------------
#define D_APPS_HEADER               "Apps"
#define D_APPS_NONE                 "Sin apps"

// ----------------------------------------------------------------------------
//  Bookmarks screens
// ----------------------------------------------------------------------------
#define D_BOOKMARKS_HEADER          "Marcapáginas"
#define D_BOOKMARKS_NO_BOOKS        "Sin libros"
#define D_BOOKMARKS_NONE            "Sin marcapáginas"
#define D_BOOKMARKS_OPEN_FAILED     "Error al abrir"

// ----------------------------------------------------------------------------
//  Reader
// ----------------------------------------------------------------------------
#define D_READER_BOOK_EMPTY         "Libro vacío"
#define D_READER_BACK_LIBRARY       "Volver a biblioteca"

// ----------------------------------------------------------------------------
//  App loader error overlay
// ----------------------------------------------------------------------------
#define D_APP_ERR_TITLE             "Error de app"
#define D_APP_ERR_NULL_PATH         "ruta nula"
#define D_APP_ERR_NOT_FOUND         "App no encontrada"
#define D_APP_ERR_TOO_SMALL         "App muy pequeña"
#define D_APP_ERR_INVALID_FILE      "Archivo inválido"
#define D_APP_ERR_TOO_LARGE         "App muy grande"
#define D_APP_ERR_SIZE_LIMIT        "> 48 KB"
#define D_APP_ERR_READ              "Error de lectura"
#define D_APP_ERR_PARTIAL_READ      "Lectura parcial"
#define D_APP_ERR_NO_EXEC_MEM       "Sin memoria ejec."
#define D_APP_ERR_BAD_FILE          "App inválida"
#define D_APP_ERR_WRONG_MAGIC       "Firma incorrecta"
#define D_APP_ERR_API_MISMATCH      "API incompatible"
#define D_APP_ERR_API_FMT           "API v%u, requiere v%u"
#define D_APP_ERR_BAD_ENTRY         "Entrada inválida"
#define D_APP_ERR_BAD_RELOC         "Tabla reloc inválida"
#define D_APP_ERR_RELOC_RANGE       "Reloc fuera de rango"

// ----------------------------------------------------------------------------
//  Bookmark add toasts
// ----------------------------------------------------------------------------
#define D_TOAST_BOOKMARK_EXISTS     "Marcapáginas ya existe"
#define D_TOAST_BOOKMARK_SAVED      "Marcapáginas guardado"

// ============================================================================
//  Web UI
// ============================================================================

// ----------------------------------------------------------------------------
//  Shared chrome / storage card
// ----------------------------------------------------------------------------
#define D_WEB_STORAGE_HEADING       "Almacenamiento"
#define D_WEB_STORAGE_BOOKS         "Libros"
#define D_WEB_STORAGE_USED          "Usado"
#define D_WEB_STORAGE_FREE          "Libre"
#define D_WEB_STORAGE_TOTAL         "Total"
#define D_WEB_STORAGE_PCT_SUFFIX    "% del almacenamiento interno en uso."

// ----------------------------------------------------------------------------
//  Navigation links
// ----------------------------------------------------------------------------
#define D_WEB_NAV_HOME              "Inicio"
#define D_WEB_NAV_FILES             "Archivos"
#define D_WEB_NAV_BOOKMARKS         "Marcapáginas"
#define D_WEB_NAV_LIST              "Lista"
#define D_WEB_NAV_SETTINGS          "Ajustes"
#define D_WEB_NAV_FACTORY_RESET     "Reinicio de fábrica"
#define D_WEB_NAV_BACK              "Atrás"

// ----------------------------------------------------------------------------
//  Home page
// ----------------------------------------------------------------------------
#define D_WEB_HOME_TITLE            "Pala One"
#define D_WEB_HOME_FW_PREFIX        "Firmware "
#define D_WEB_HOME_MIDDOT_SEP       " &middot; "
#define D_WEB_HOME_BOOKS_SUFFIX     " libros"
#define D_WEB_HOME_FREE_LABEL       "Libre: "
#define D_WEB_HOME_STORAGE_WARN     "&#9888; Almacenamiento no disponible o casi lleno. Si las subidas fallan, elimine libros o use Reinicio de fábrica desde esta interfaz web."
#define D_WEB_UPLOAD_BOOK_HEADING   "Subir libro"
#define D_WEB_UPLOAD_BOOK_DESC      "Envíe archivos UTF-8 de texto plano a <b>/books</b> en el dispositivo, luego organícelos en carpetas desde la página Archivos."
#define D_WEB_UPLOAD_BOOK_BUTTON    "Subir"
#define D_WEB_MANAGE_FILES_BUTTON   "Administrar archivos"
#define D_WEB_INSTALL_APP_HEADING   "Instalar app"
#define D_WEB_INSTALL_APP_DESC      "Suba un binario de app Pala (<b>.bin</b>) a <b>/apps</b>. El encabezado se valida antes de confirmar; solo se aceptan archivos con la firma y versión de API correctas. Abra <b>Apps</b> desde la biblioteca para ejecutarla."
#define D_WEB_INSTALL_APP_BUTTON    "Instalar app"
#define D_WEB_NOTES_HEADING         "Notas"
#define D_WEB_NOTES_DESC            "Los libros subidos se normalizan y compactan antes de guardarse, por lo que un TXT de origen puede ser más grande que el archivo final almacenado. El lector está optimizado para texto plano UTF-8 e idiomas con alfabeto latino."

// ----------------------------------------------------------------------------
//  Files page
// ----------------------------------------------------------------------------
#define D_WEB_FILES_HEADING         "Archivos"
#define D_WEB_FILES_SUBTITLE        "Administre libros, carpetas y estructura de la biblioteca de Pala One."
#define D_WEB_CREATE_FOLDER_HEADING "Crear carpeta"
#define D_WEB_CREATE_FOLDER_PLACEHOLDER "libros o clasicos/espanol"
#define D_WEB_CREATE_FOLDER_BUTTON  "Crear carpeta"
#define D_WEB_CREATE_FOLDER_HINT    "Las carpetas viven dentro de /books."
#define D_WEB_FOLDERS_HEADING       "Carpetas"
#define D_WEB_NO_FOLDERS            "Aún no hay carpetas. Los libros viven en la raíz de /books."
#define D_WEB_CONFIRM_DELETE_FOLDER "¿Eliminar carpeta? Solo se pueden eliminar carpetas vacías."
#define D_WEB_DELETE_BUTTON         "Eliminar"
#define D_WEB_LIBRARY_FILES_HEADING "Archivos de biblioteca"
#define D_WEB_LIBRARY_FULL_WARN     "&#9888; Biblioteca llena (máx. 80 libros). Elimine libros para hacer espacio."
#define D_WEB_FOLDER_LIMIT_WARN     "&#9888; Límite de carpetas alcanzado (máx. 32)."
#define D_WEB_NO_BOOKS_UPLOADED     "Aún no se han subido libros."
#define D_WEB_BOOK_ROOT             "Raíz"
#define D_WEB_BOOK_BYTES_LABEL      " bytes"
#define D_WEB_BOOK_FOLDER_LABEL     " &middot; carpeta: "
#define D_WEB_BOOK_CURRENT_PAGE     " &middot; página actual: "
#define D_WEB_JUMP_BUTTON           "Ir"
#define D_WEB_JUMP_HINT             "Establezca la página que se abrirá la próxima vez en el dispositivo."
#define D_WEB_JUMP_HINT2            "La primera apertura puede tardar un momento."
#define D_WEB_PAGE_PLACEHOLDER      "Página"
#define D_WEB_MOVE_BUTTON           "Mover"
#define D_WEB_MOVE_HINT             "Use la ruta exacta de la carpeta."
#define D_WEB_MOVE_PLACEHOLDER      "vacío para raíz"
#define D_WEB_CONFIRM_DELETE_FILE   "¿Eliminar archivo?"
#define D_WEB_APPS_PAGE_HEADING     "Apps"
#define D_WEB_NO_APPS_INSTALLED     "Sin apps instaladas."
#define D_WEB_CONFIRM_DELETE_APP    "¿Eliminar app?"

// ----------------------------------------------------------------------------
//  Plain-text 4xx/5xx error bodies
// ----------------------------------------------------------------------------
#define D_WEB_ERR_MISSING_ID            "id faltante"
#define D_WEB_ERR_BAD_ID                "id inválido"
#define D_WEB_ERR_MISSING_FOLDER        "carpeta faltante"
#define D_WEB_ERR_BAD_FOLDER            "carpeta inválida"
#define D_WEB_ERR_FOLDER_LIMIT          "límite de carpetas alcanzado"
#define D_WEB_ERR_MKDIR_FAILED          "fallo al crear carpeta"
#define D_WEB_ERR_FOLDER_NOT_FOUND      "carpeta no encontrada"
#define D_WEB_ERR_FOLDER_NOT_EMPTY      "carpeta no vacía"
#define D_WEB_ERR_DELETE_FAILED         "fallo al eliminar"
#define D_WEB_ERR_FOLDER_CREATE_FAILED  "fallo al crear carpeta"
#define D_WEB_ERR_DEST_EXISTS           "destino ya existe"
#define D_WEB_ERR_MOVE_FAILED           "fallo al mover"
#define D_WEB_ERR_MISSING_ID_PAGE       "id/página faltante"
#define D_WEB_ERR_MISSING_BOOK_IDX      "libro/idx faltante"
#define D_WEB_ERR_BAD_BOOK              "libro inválido"
#define D_WEB_ERR_BAD_IDX               "idx inválido"
#define D_WEB_ERR_MISSING_BOOK          "libro faltante"
#define D_WEB_ERR_MISSING_NAME          "nombre faltante"
#define D_WEB_ERR_INVALID_NAME          "nombre inválido"

// ----------------------------------------------------------------------------
//  List page
// ----------------------------------------------------------------------------
#define D_WEB_LIST_HEADING          "Lista"
#define D_WEB_LIST_SUBTITLE         "Cree una lista simple de compras o tareas para Pala One."
#define D_WEB_LIST_EDIT_HEADING     "Editar lista"
#define D_WEB_LIST_EDIT_DESC        "Los elementos aparecen en el dispositivo solo cuando al menos una línea contiene texto. Mantenga el botón en el dispositivo para marcar un elemento como hecho."
#define D_WEB_LIST_ITEM_PLACEHOLDER "Elemento de lista"
#define D_WEB_LIST_SAVE_BUTTON      "Guardar lista"
#define D_WEB_LIST_DELETE_DONE      "Eliminar elementos marcados"
#define D_WEB_LIST_HINT             "Las filas vacías se ignoran. Las filas marcadas se pueden eliminar directamente."

// ----------------------------------------------------------------------------
//  Reset page
// ----------------------------------------------------------------------------
#define D_WEB_RESET_HEADING         "Reinicio de fábrica"
#define D_WEB_RESET_SUBTITLE        "Borre todos los libros, marcapáginas, progreso y recursos personalizados."
#define D_WEB_RESET_CONFIRM_HEADING "Confirmar reinicio"
#define D_WEB_RESET_WARNING         "Esto eliminará TODOS los libros, marcapáginas y progreso de lectura."
#define D_WEB_RESET_DETAIL          "El sistema de archivos del dispositivo se formateará y los ajustes volverán a los valores predeterminados."
#define D_WEB_RESET_YES_BUTTON      "Sí, reiniciar"
#define D_WEB_RESET_COMPLETE_HEADING "Reinicio de fábrica completo"
#define D_WEB_RESET_COMPLETE_DESC   "Todos los libros, marcapáginas, progreso y recursos personalizados fueron eliminados. El dispositivo está ahora en un estado limpio."
#define D_WEB_GO_HOME_BUTTON        "Ir al inicio"
#define D_WEB_OPEN_FILES_BUTTON     "Abrir archivos"
#define D_WEB_RESET_SUCCESS_TITLE   "Reinicio completo"
#define D_WEB_RESET_SUCCESS_SUBTITLE "Pala One se reinició con éxito."
#define D_WEB_RESET_BANNER          "&#10003; Reinicio de fábrica completo."

// ----------------------------------------------------------------------------
//  Settings page
// ----------------------------------------------------------------------------
#define D_WEB_SETTINGS_TITLE        "Ajustes de Pala One"
#define D_WEB_SETTINGS_SUBTITLE_PREFIX "Firmware "
#define D_WEB_SETTINGS_SUBTITLE_SUFFIX " — página de configuración almacenada directamente en el dispositivo."
#define D_WEB_SETTINGS_BACK_NAV     "&#8592; Inicio"
#define D_WEB_READING_HEADING       "Lectura"
#define D_WEB_FONT_SIZE_LABEL       "Tamaño de fuente"
#define D_WEB_FONT_SIZE_8           "8px &mdash; diminuto"
#define D_WEB_FONT_SIZE_10          "10px &mdash; pequeño"
#define D_WEB_FONT_SIZE_12          "12px &mdash; mediano"
#define D_WEB_FONT_SIZE_14          "14px &mdash; grande"
#define D_WEB_FONT_SIZE_HINT        "Controla cuántas líneas caben en cada página."
#define D_WEB_SLEEP_AFTER_LABEL     "Suspender después de"
#define D_WEB_SLEEP_30S             "30 segundos"
#define D_WEB_SLEEP_1M              "1 minuto"
#define D_WEB_SLEEP_2M              "2 minutos"
#define D_WEB_SLEEP_5M              "5 minutos"
#define D_WEB_SLEEP_10M             "10 minutos"
#define D_WEB_SLEEP_30M             "30 minutos"
#define D_WEB_SLEEP_HINT            "La suspensión automática mantiene bajo el consumo de batería en reposo."
#define D_WEB_LINE_SPACING_LABEL    "Espaciado de línea"
#define D_WEB_LINE_SPACING_0        "0 px &mdash; compacto"
#define D_WEB_LINE_SPACING_1        "1 px &mdash; normal"
#define D_WEB_LINE_SPACING_2        "2 px &mdash; relajado"
#define D_WEB_LINE_SPACING_3        "3 px &mdash; suelto"
#define D_WEB_LINE_SPACING_HINT     "Un pequeño cambio aquí puede facilitar mucho la lectura."
#define D_WEB_SAVE_SETTINGS_BUTTON  "Guardar ajustes"
#define D_WEB_SETTINGS_NO_EXTRAS    "Sin archivos extra, scripts ni fuentes."
#define D_WEB_SCREENSAVER_HEADING   "Salvapantallas"
#define D_WEB_SCREENSAVER_SPECS     "Suba bytes XBM en bruto: <b>3904 bytes</b>, 250&times;122 px, 1 bit, LSB primero, 32 bytes por fila."
#define D_WEB_SCREENSAVER_TIP       "Consejo: use <a class='link' href='https://javl.github.io/image2cpp/' target='_blank'>image2cpp</a> con <b>Plain bytes</b>. Invierta los colores si es necesario."
#define D_WEB_SCREENSAVER_ACTIVE    "&#10003; Salvapantallas personalizado activo."
#define D_WEB_CONFIRM_DEL_SCREENSAVER "¿Eliminar salvapantallas personalizado?"
#define D_WEB_SCREENSAVER_DEFAULT   "Usando salvapantallas predeterminado."
#define D_WEB_SLEEP_IMAGE_LABEL     "Archivo de imagen de suspensión"
#define D_WEB_SCREENSAVER_UPLOAD_BUTTON "Subir imagen"

// ----------------------------------------------------------------------------
//  Upload routes
// ----------------------------------------------------------------------------
#define D_WEB_UPLOAD_COMPLETE_HEADING "Subida completa"
#define D_WEB_UPLOAD_COMPLETE_DESC  "Su libro está ahora almacenado en el dispositivo y disponible en la biblioteca."
#define D_WEB_UPLOAD_BOOK_LABEL     "Libro"
#define D_WEB_UPLOAD_STORED_SIZE    "Tamaño almacenado"
#define D_WEB_UPLOAD_BOOKS_NOW      "Libros ahora"
#define D_WEB_UPLOAD_FREE_SPACE     "Espacio libre"
#define D_WEB_UPLOAD_ANOTHER        "Subir otro"
#define D_WEB_UPLOAD_BOOK_SAVED     "Libro guardado con éxito."
#define D_WEB_UPLOAD_FINISHED       "&#10003; Subida finalizada."
#define D_WEB_UPLOAD_ERR_FALLBACK   "Subida fallida"
#define D_WEB_ERR_LIBRARY_FULL      "Biblioteca llena"
#define D_WEB_ERR_NOT_ENOUGH_SPACE  "Espacio insuficiente"
#define D_WEB_ERR_CANT_CREATE_TEMP_BOOK "No se pudo crear archivo temporal de subida"
#define D_WEB_ERR_WRITE_FAILED      "Fallo de escritura (¿sin espacio?)"
#define D_WEB_ERR_FINALIZE_UPLOAD   "Fallo al finalizar la subida"
#define D_WEB_ERR_EMPTY_UPLOAD      "Subida vacía"
#define D_WEB_ERR_UPLOAD_ABORTED    "Subida abortada"
#define D_WEB_SLEEP_UPLOAD_ERR_FALLBACK "Fallo al subir imagen de suspensión"
#define D_WEB_SLEEP_UPLOAD_HEADING  "Salvapantallas actualizado"
#define D_WEB_SLEEP_UPLOAD_DESC     "Su imagen de suspensión personalizada se guardó con éxito y se mostrará la próxima vez que el dispositivo se suspenda."
#define D_WEB_BACK_TO_SETTINGS      "Volver a ajustes"
#define D_WEB_SLEEP_UPLOAD_SUBTITLE "Salvapantallas guardado con éxito."
#define D_WEB_SLEEP_UPLOAD_BANNER   "&#10003; Imagen de suspensión personalizada subida."
#define D_WEB_SLEEP_ERR_TEMP        "No se pudo crear archivo temporal de suspensión"
#define D_WEB_SLEEP_ERR_SIZE        "La imagen de suspensión debe ser exactamente 3904 bytes"
#define D_WEB_SLEEP_ERR_SAVE        "Fallo al guardar imagen de suspensión"
#define D_WEB_SLEEP_ERR_ABORTED     "Subida de imagen de suspensión abortada"

// ----------------------------------------------------------------------------
//  App upload route
// ----------------------------------------------------------------------------
#define D_WEB_APP_UPLOAD_ERR_FALLBACK "Subida de app fallida"
#define D_WEB_APP_INSTALLED_HEADING "App instalada"
#define D_WEB_APP_INSTALLED_DESC    "Abra el dispositivo, vaya a <b>Apps</b> en la biblioteca y haga doble clic para ejecutarla."
#define D_WEB_APP_LABEL             "App"
#define D_WEB_APPS_NOW              "Apps ahora"
#define D_WEB_APP_INSTALLED_SUBTITLE "App guardada en /apps/."
#define D_WEB_APP_INSTALLED_BANNER  "&#10003; App lista para ejecutarse."
#define D_WEB_APP_VALID_OK          "OK"
#define D_WEB_APP_VALID_TOO_SMALL   "App inválida (archivo muy pequeño)"
#define D_WEB_APP_VALID_BAD_MAGIC   "App inválida (firma incorrecta)"
#define D_WEB_APP_VALID_BAD_ENTRY   "App inválida (entrada inválida)"
#define D_WEB_APP_VALID_BAD_RELOC   "App inválida (tabla reloc inválida)"
#define D_WEB_APP_VALID_API_FMT     "App inválida (API v%u, requiere v%u)"
#define D_WEB_APP_VALID_INVALID     "App inválida"
#define D_WEB_APPS_DIR_FULL         "Directorio de apps lleno"
#define D_WEB_ERR_CANT_CREATE_TEMP_APP "No se pudo crear archivo temporal de app"
#define D_WEB_APP_TOO_LARGE         "App muy grande (> 48 KB)"
#define D_WEB_APP_BINARY_TOO_SMALL  "Binario de app muy pequeño"
#define D_WEB_APP_CANT_READ_HEADER  "No se pudo leer el encabezado de la app"
#define D_WEB_APP_FINALIZE_FAILED   "Fallo al finalizar la subida de la app"
#define D_WEB_APP_UPLOAD_ABORTED    "Subida de app abortada"

// ----------------------------------------------------------------------------
//  Bookmarks web page
// ----------------------------------------------------------------------------
#define D_WEB_BOOKMARKS_HEADING     "Marcapáginas"
#define D_WEB_BOOKMARKS_SUBTITLE    "Posiciones de lectura guardadas para Pala One, agrupadas por libro."
#define D_WEB_NO_BOOKS_YET          "Aún no hay libros disponibles."
#define D_WEB_NO_BOOKMARKS_CARD     "Sin marcapáginas"
#define D_WEB_BOOKMARKS_OPEN_FAILED_CARD "Error al abrir"
#define D_WEB_BOOKMARK_PILL_PREFIX  "Marcapáginas "
#define D_WEB_BOOKMARK_VIEW         "Ver"
#define D_WEB_CONFIRM_DELETE_BOOKMARK "¿Eliminar marcapáginas?"
#define D_WEB_BOOKMARK_DOWNLOAD_ALL "Descargar todos los marcapáginas"
#define D_WEB_BOOKMARK_VIEW_HEADING "Vista de marcapáginas"
#define D_WEB_BOOKMARK_VIEW_SUBTITLE "Previsualice el texto de página guardado para este marcapáginas."
#define D_WEB_BOOKMARK_VIEW_BACK_NAV "&#8592; Atrás"
#define D_WEB_BOOKMARK_PAGE_EMPTY   "(vacío)"
#define D_WEB_BOOKMARK_OPEN_FAILED_DOT "Error al abrir."

// Bookmark export plaintext labels
#define D_WEB_BMEXPORT_BOOK         "Libro: "
#define D_WEB_BMEXPORT_BOOKMARKS    "Marcapáginas: "
#define D_WEB_BMEXPORT_BOOKMARK_LBL "Marcapáginas "
#define D_WEB_NO_BOOKMARKS_THIS_BOOK "Sin marcapáginas para este libro"

#endif  // PALA_LANG_ES_LA_H
